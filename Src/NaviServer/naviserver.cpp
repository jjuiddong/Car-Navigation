
#include "stdafx.h"
#include "naviserver.h"

using namespace network2;


cNaviServer::cNaviServer()
{
}

cNaviServer::~cNaviServer()
{
	Clear();
}


bool cNaviServer::Init(network2::cNetController &netController)
{
	common::cConfig config(g_configFileName);

	const string ip = config.GetString("naviserver_ip", "127.0.0.1");
	const int port = config.GetInt("naviserver_port", 10001);

	m_svr.AddProtocolHandler(this);

	if (!netController.StartTcpServer(&m_svr, port))
		return false;

	return true;
}


bool cNaviServer::Update(const float deltaSeconds)
{

	return true;
}


// gps protocol handler
bool cNaviServer::GPSInfo(gps::GPSInfo_Packet &packet)
{
	if (m_dateStr != packet.date)
	{
		// check path folder
		if (!StrPath("./path").IsFileExist())
			::CreateDirectoryA("./path", nullptr);

		m_pathFilename = "./path/path_";
		m_pathFilename += packet.date;
		m_pathFilename += ".txt";

		m_dateStr = packet.date;
	}

	const Vector2d pos(packet.lon, packet.lat);
	if (m_prevGpsPos != pos)
	{
		dbg::Logp2(m_pathFilename.c_str(), "%s, %.15f, %.15f\n"
			, packet.date.c_str(), packet.lon, packet.lat);
		m_prevGpsPos = pos;
	}

	std::cout << "recv GPSInfo" << std::endl;

	return true;
}


void cNaviServer::Clear()
{
	m_svr.Close();
}
