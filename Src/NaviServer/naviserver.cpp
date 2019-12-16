
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
	const string dateStr = common::GetCurrentDateTime4();

	if (m_dateStr != dateStr)
	{
		// check path folder
		if (!StrPath("./path").IsFileExist())
			::CreateDirectoryA("./path", nullptr);

		m_pathFilename = "./path/path_";
		m_pathFilename += dateStr;
		m_pathFilename += ".txt";

		m_dateStr = dateStr;
	}

	const Vector2d pos(packet.lon, packet.lat);
	if (m_prevGpsPos != pos)
	{
		const string ctime = common::GetCurrentDateTime();
		dbg::Logp2(m_pathFilename.c_str(), "%s, %.15f, %.15f\n"
			, ctime.c_str(), packet.lon, packet.lat);
		m_prevGpsPos = pos;
	}

	return true;
}


void cNaviServer::Clear()
{
	m_svr.Close();
}
