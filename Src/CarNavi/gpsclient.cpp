
#include "stdafx.h"
#include "gpsclient.h"
#include "carnavi.h"
#include "mapview.h"


cGpsClient::cGpsClient()
	: m_client(new network2::cPacketHeaderNoFormat())
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

	//GpsReplay();
	//ReadPathFile("path/path_20200504.txt");
	//PathFileReplay();

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

		memcpy(m_recvStr.m_str, packet.m_data, min(m_recvStr.SIZE-1, (uint)packet.m_writeIdx));
		m_recvStr.m_str[min(m_recvStr.SIZE - 1, (uint)packet.m_writeIdx)] = NULL;

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
			const uint len = m_serial.RecvData((BYTE*)m_recvStr.m_str
				, m_recvStr.SIZE - 1);
			if (len == 0)
				break;

			m_recvStr.m_str[min(m_recvStr.SIZE - 1, len)] = NULL;
			m_recvTime = curT;

			const string date = common::GetCurrentDateTime();
			dbg::Logp2("gps.txt", "%s, %s", date.c_str(), m_recvStr.c_str());

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

	case eInputType::GpsFile:
	{
		const float updateTime = 0.005f;
		if (m_gpsFs.is_open()
			&& !m_gpsFs.eof()
			&& ((curT - m_recvTime) > updateTime))
		{
			m_recvTime = curT;

			while (1)
			{
				string line;
				if (!getline(m_gpsFs, line))
					break;

				m_recvStr = line;

				gis::sGPRMC tmp;
				if (ParseStr(m_recvStr, tmp))
				{
					isRead = true;
					out = tmp;
					m_recvCount++;
					break;
				}
			}
		}
	}
	break;

	case eInputType::PathFile:
	{
		if (0 == m_gpsInfo.date)
		{
			if (GetGpsInfoFromFile(m_gpsInfo))
			{
				isRead = true;
				out = m_gpsInfo;
				m_prevDateTime = cDateTime2::Now();
				m_prevGpsDateTime.SetTime(m_gpsInfo.date);

				if (!GetGpsInfoFromFile(m_gpsInfo))
					m_gpsInfo.date = 0; // end of file
			}
		}
		else
		{
			const cDateTime2 curDateTime = cDateTime2::Now();
			cDateTime2 dt1 = curDateTime - m_prevDateTime;
			const cDateTime2 dt2 = cDateTime2(m_gpsInfo.date) - m_prevGpsDateTime;
			dt1.m_t *= 2; // x2 speed up

			if (dt2 < dt1)
			{
				isRead = true;
				out = m_gpsInfo;
				m_prevDateTime = curDateTime;
				m_prevGpsDateTime.SetTime(m_gpsInfo.date);

				if (!GetGpsInfoFromFile(m_gpsInfo))
					m_gpsInfo.date = 0; // end of file
			}
		}
	}
	break;

	default: assert(0); break;
	}

	return isRead;
}


bool cGpsClient::GpsReplay()
{
	m_gpsFs = std::ifstream("gps.txt");
	if (!m_gpsFs.is_open())
	{
		m_inputType = eInputType::Serial;
		return false;
	}

	m_inputType = eInputType::GpsFile;

	return true;
}


bool cGpsClient::PathFileReplay()
{
	m_inputType = eInputType::PathFile;
	m_fileAnimationIdx = 0;
	m_gpsInfo.date = 0;
	return true;
}


bool cGpsClient::StopPathFileReplay()
{
	m_inputType = eInputType::Serial;
	m_fileAnimationIdx = 0; 
	return true;
}


bool cGpsClient::IsPathReplay()
{
	return (eInputType::PathFile == m_inputType);
}


bool cGpsClient::IsGpsReplay()
{
	return (eInputType::GpsFile == m_inputType);
}


bool cGpsClient::IsServer()
{
	return eInputType::Network == m_inputType;
}


// Path 파일에서 GPS정보를 읽어온다.
bool cGpsClient::GetGpsInfoFromFile(OUT gis::sGPRMC &out)
{
	if (m_fileAnimationIdx >= (int)m_pathReplayData.size())
		return false;

	static Vector2d oldLonLat;
	while (m_fileAnimationIdx < (int)m_pathReplayData.size())
	{
		const sPath &path = m_pathReplayData[m_fileAnimationIdx++];
		if (path.lonLat.IsEmpty())
			continue;

		if (oldLonLat != path.lonLat)
		{
			out.available = true;
			out.date = path.t;
			out.lonLat = path.lonLat;
			out.speed = path.speed;
			oldLonLat = path.lonLat;
			return true;
		}
	}
	return false;
}


bool cGpsClient::IsConnect()
{
	switch (m_inputType)
	{
	case eInputType::Network:
		return m_client.IsConnect();
	case eInputType::Serial:
		return m_serial.IsOpen();
	case eInputType::GpsFile:
		return true;
	case eInputType::PathFile:
		return true;
	default: assert(0); break;
	}
	return false;
}


bool cGpsClient::IsReadyConnect()
{
	return m_client.IsReadyConnect();
}


// parse NMEA protocol
// $GPRMC
//	- lon/lat
//	- speed
// $GPATT
// $GPVTG
// $GPGGA
//	- altitude
// $GPGSA
// reference
//	- http://www.ndgps.go.kr/_prog/_board/index.php?mode=V&no=696&code=dgps1&site_dvs_cd=kr&menu_dvs_cd=&skey=&sval=&GotoPage=8
//	- https://m.blog.naver.com/PostView.nhn?blogId=thefeel777&logNo=130109531670&proxyReferer=https:%2F%2Fwww.google.com%2F
//  - http://aprs.gids.nl/nmea/
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

	m_pathReplayData.clear();
	m_pathReplayData.reserve(1024);

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
		if (out.size() > 3)
			info.speed = (float)atof(out[3].c_str());

		if (info.lonLat.IsEmpty())
			continue;

		m_pathReplayData.push_back(info);
	}

	return true;
}


void cGpsClient::Clear()
{
	m_client.Close();
}
