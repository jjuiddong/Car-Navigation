
#include "stdafx.h"
#include "gpsclient.h"


cGpsClient::cGpsClient()
	: m_client(new network2::cPacketHeaderNoFormat())
	, m_state(eState::Server)
	, m_ip("192.168.1.102")
	, m_port(60660)
	, m_recvTime(0)
	, m_recvCount(0)
	, m_fileAnimationIdx(0)
{
	m_timer.Create();
}

cGpsClient::~cGpsClient()
{	
}


bool cGpsClient::ConnectGpsServer(const Str16 &ip, const int port)
{
	m_client.Close();

	m_ip = ip;
	m_port = port;
	m_client.Init(ip, port);

	return true;
}


bool cGpsClient::GetGpsInfo(OUT gis::sGPRMC &out)
{
	const float ALIVE_TIME = 10.f;

	if (eState::PathFile == m_state)
		return GetGpsInfoFromFile(out);

	if (!IsConnect())
	{
		m_recvTime = 0.f;
		return false;
	}

	// 일정시간동안 패킷을 받지못했다면 커넥션을 끊는다.
	const double curT = m_timer.GetSeconds();
	if (m_recvTime == 0.f)
		m_recvTime = curT;

	network2::cPacket packet(m_client.m_packetHeader);
	if (!m_client.m_recvQueue.Front(packet))
	{
		if (curT - m_recvTime > ALIVE_TIME)
			m_client.Close();
		return false;
	}

	m_recvTime = curT;
	m_recvCount++;
	memcpy(m_recvStr.m_str, packet.m_data, min(m_recvStr.SIZE, (uint)packet.m_writeIdx));

	std::ofstream ofsGps("gps.txt", std::ios::app);
	ofsGps << m_recvStr.c_str();

	return ParseStr(m_recvStr, out);
}


bool cGpsClient::FileReplay() 
{
	m_state = eState::PathFile;
	return true;
}


// 파일에서 GPS정보를 읽어온다.
bool cGpsClient::GetGpsInfoFromFile(OUT gis::sGPRMC &out)
{
	if (m_fileAnimationIdx >= (int)m_paths.size())
		return false;

	static Vector2d oldLonLat;
	while (m_fileAnimationIdx < (int)m_paths.size())
	{
		const Vector2d lonLat = m_paths[m_fileAnimationIdx++].lonLat;
		if (lonLat.IsEmpty())
			continue;

		if (oldLonLat != lonLat)
		{
			out.available = true;
			out.lonLat = lonLat;
			oldLonLat = lonLat;
			break;
		}
	}
	out.speed = 0.f;
	return true;
}


bool cGpsClient::IsConnect()
{
	return m_client.IsConnect();
}


bool cGpsClient::IsReadyConnect()
{
	return m_client.IsReadyConnect();
}


bool cGpsClient::ParseStr(const Str512 &str, OUT gis::sGPRMC &out)
{
	vector<string> lines;
	common::tokenizer(str.m_str, "\n", "", lines);

	out.available = false;
	for (auto &line : lines)
	{
		gis::sGPRMC gps;
		if (gis::GetGPRMCLonLat(line.c_str(), gps))
			out = gps;
	}

	if (!out.available)
		return false;
	
	if (out.lonLat.IsEmpty())
	{
		out.available = false;
		return false;
	}

	return true;
}


// path.txt 파일을 로딩한다.
bool cGpsClient::ReadPathFile(const char *fileName)
{
	using namespace std;
	ifstream ifs(fileName);
	if (!ifs.is_open())
		return false;

	m_paths.clear();
	m_paths.reserve(1024);

	string line;
	while (getline(ifs, line))
	{
		vector<string> out;
		common::tokenizer(line, ",", "", out);
		if (out.size() < 3)
			continue;

		sPath info;
		info.t = common::GetCurrentDateTime6(out[0]);
		info.lonLat.x = atof(out[1].c_str());
		info.lonLat.y = atof(out[2].c_str());
		m_paths.push_back(info);
	}

	return true;
}


void cGpsClient::Clear()
{
	m_client.Close();
}
