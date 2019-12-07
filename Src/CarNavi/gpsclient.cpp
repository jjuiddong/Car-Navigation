
#include "stdafx.h"
#include "gpsclient.h"
#include "carnavi.h"
#include "mapview.h"


cGpsClient::cGpsClient()
	: m_client(new network2::cPacketHeaderNoFormat())
	, m_state(eState::Server)
	, m_ip("192.168.1.102")
	, m_port(60660)
	, m_recvTime(0)
	, m_speedRecvTime(0)
	, m_recvCount(0)
	, m_speed(0.f)
	, m_fileAnimationIdx(0)
	, m_inputType(eInputType::Serial)
{
	m_timer.Create();
	m_paths.reserve(1024);
}

cGpsClient::~cGpsClient()
{
}


bool cGpsClient::Init()
{
	m_ip = g_global->m_config.GetString("gps_server_ip", "192.168.1.102");
	m_port = g_global->m_config.GetInt("gps_server_port", 60660);
	m_inputType = (eInputType)g_global->m_config.GetInt("gps_input_type", 0);

	return true;
}


bool cGpsClient::ConnectGpsServer(const Str16 &ip, const int port)
{
	m_client.Close();

	m_ip = ip;
	m_port = port;
	m_client.Init(ip, port);

	g_global->m_config.SetValue("gps_server_ip", ip.c_str());
	g_global->m_config.SetValue("gps_server_port", port);
	g_global->m_config.SetValue("gps_input_type", (int)m_inputType);

	return true;
}


bool cGpsClient::ConnectGpsSerial(const int portNum, const int baudRate)
{
	const bool result = m_serial.Open(portNum, baudRate, '\n');
	g_global->m_config.SetValue("gps_input_type", (int)m_inputType);
	return result;
}


bool cGpsClient::GetGpsInfo(OUT gis::sGPRMC &out)
{
	if (eState::PathFile == m_state)
		return false;

	if (!IsConnect())
	{
		m_recvTime = 0.f;
		m_speedRecvTime = 0.f;
		return false;
	}

	const float ALIVE_TIME = 10.f;

	// 일정시간동안 패킷을 받지못했다면 커넥션을 끊는다. (Network type)
	const double curT = m_timer.GetSeconds();
	if (m_recvTime == 0.f)
		m_recvTime = curT;

	bool isRead = false;

	switch (m_inputType)
	{
	case eInputType::Network:
	{
		network2::cPacket packet(m_client.m_packetHeader);
		if (!m_client.m_recvQueue.Front(packet))
		{
			if (curT - m_recvTime > ALIVE_TIME)
				m_client.Close(); // long time disconnectio, close network
			return false;
		}

		memcpy(m_recvStr.m_str, packet.m_data, min(m_recvStr.SIZE, (uint)packet.m_writeIdx));
		if (ParseStr(m_recvStr, out))
		{
			if (out.speed == 0.f)
			{
				const double dt = curT - m_speedRecvTime;
				if (dt < 3.f)
					out.speed = m_speed;
			}
			else
			{
				m_speed = out.speed;
				m_speedRecvTime = curT;
			}

			isRead = true;
			m_recvCount++;
		}

		m_recvTime = curT;
		dbg::Logp2("gps.txt", m_recvStr.c_str());
	}
	break;

	case eInputType::Serial:
	{
		while (1)
		{
			//int len = 0;
			//const bool result = m_serial.m_serial.ReadStringUntil('\n'
			//	, m_recvStr.m_str, len, m_recvStr.SIZE);
			//if (!result || (len <= 0))
			//	break;
			const uint len = m_serial.RecvData((BYTE*)m_recvStr.m_str
				, m_recvStr.SIZE);
			if (len == 0)
				break;

			if (m_recvStr.SIZE > len)
				m_recvStr.m_str[len] = NULL;

			m_recvTime = curT;

			dbg::Logp2("gps.txt", m_recvStr.c_str());

			gis::sGPRMC tmp;
			if (ParseStr(m_recvStr, tmp))
			{
				isRead = true;
				out = tmp;
				m_recvCount++;
			}
		}

		if (curT - m_recvTime > ALIVE_TIME)
			m_serial.Close();
	}
	break;

	default: assert(0); break;
	}

	return isRead;
}


bool cGpsClient::FileReplay() 
{
	m_state = eState::PathFile;
	return true;
}


bool cGpsClient::StopFileReplay()
{
	m_state = eState::Server;
	m_fileAnimationIdx = 0; 
	return true;
}


bool cGpsClient::IsFileReplay()
{
	return eState::PathFile == m_state;
}


bool cGpsClient::IsServer()
{
	return eState::Server == m_state;
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
	switch (m_inputType)
	{
	case eInputType::Network:
		return m_client.IsConnect();
	case eInputType::Serial:
		return m_serial.IsOpen();
	default: assert(0); break;
	}
	return false;
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

	int state = 0;
	gis::sGPRMC gps;
	ZeroMemory(&gps, sizeof(gps));
	for (auto &line : lines)
	{
		if (gis::GetGPRMCLonLat(line.c_str(), gps))
		{
			out = gps;
			state = 1;
		}
		else if ((state != 1) 
			&& (gis::GetGPATTLonLat(line.c_str(), gps)))
		{
			out = gps;
		}
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


// path 파일을 로딩한다.
bool cGpsClient::ReadPathFile(const char *fileName)
{
	using namespace std;
	ifstream ifs(fileName);
	if (!ifs.is_open())
		return false;

	m_paths.clear();
	m_paths.reserve(1024);

	int cnt = 0;
	string line;
	while (getline(ifs, line) 
		//&& (cnt++ < 10000)
		)
	{
		vector<string> out;
		common::tokenizer(line, ",", "", out);
		if (out.size() < 3)
			continue;

		sPath info;
		info.t = common::GetCurrentDateTime6(out[0]);
		info.lonLat.x = atof(out[1].c_str());
		info.lonLat.y = atof(out[2].c_str());
		if (info.lonLat.IsEmpty())
			continue;

		m_paths.push_back(info);
	}

	return true;
}


void cGpsClient::Clear()
{
	m_client.Close();
}
