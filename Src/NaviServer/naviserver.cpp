
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

	// create server
	const string ip = config.GetString("naviserver_ip", "127.0.0.1");
	const int port = config.GetInt("naviserver_port", 10001);

	m_svr.AddProtocolHandler(this);

	if (!netController.StartTcpServer(&m_svr, port))
		return false;

	// connect db server
	const string dbIp = config.GetString("db_ip", "127.0.0.1");
	const int dbPort = config.GetInt("db_port", 10002);
	const string dbId = config.GetString("db_id", "root");
	const string dbPasswd = config.GetString("db_passwd", "root");
	const string dbName = config.GetString("db_name", "db");
	if (!m_sqlCon.Connect(dbIp, dbPort, dbId, dbPasswd, dbName))
	{
		std::cout << "DB Connection Error\n";
	}

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
	const Str32 strDateTime = common::GetCurrentDateTime5(); // yyyy-mm-dd hh:mm:ss

	if (m_dateStr != dateStr)
	{
		// check path folder
		if (!StrPath("./path").IsFileExist())
			::CreateDirectoryA("./path", nullptr);

		m_pathFilename = "./path/path_";
		m_pathFilename += dateStr;
		m_pathFilename += ".txt";

		m_dateStr = dateStr;
		m_journeyTimeId = common::GetCurrentDateTime3();

		// upload db journey_date
		if (m_sqlCon.IsConnected())
		{
			const int userId = 1;

			const string sql =
				common::format("INSERT INTO journey_date (date, user_id, time_id"
					", distance, journey_time)"
					" VALUES ('%s', '%d', '%I64u', '%f', '%f');"
					, strDateTime.c_str(), userId, m_journeyTimeId, 0, 0);

			MySQLQuery query(&m_sqlCon, sql);
			query.ExecuteInsert();
		}
	}

	const Vector2d pos(packet.lon, packet.lat);
	if (m_prevGpsPos != pos)
	{
		const string ctime = common::GetCurrentDateTime();
		dbg::Logp2(m_pathFilename.c_str(), "%s, %.15f, %.15f, %f, %f\n"
			, ctime.c_str(), packet.lon, packet.lat, packet.speed, packet.altitude);
		m_prevGpsPos = pos;

		// filter lon/lat (korea)
		if ((pos.x < 123.f) || (pos.x > 133.f)
			|| (pos.y < 31.f) || (pos.y > 39.f))
		{
			// lon/lat position crack
			return true;
		}

		// upload db path
		if (m_sqlCon.IsConnected())
		{
			const int userId = 1;

			const string sql =
				common::format("INSERT INTO path (date_time, user_id, journey_time_id"
					", lon, lat, speed, altitude)"
					" VALUES ('%s', '%d', '%I64u', '%f', '%f', '%f', '%f');"
					, strDateTime.c_str()
					, userId, m_journeyTimeId, packet.lon, packet.lat
					, packet.speed, packet.altitude);

			MySQLQuery query(&m_sqlCon, sql);
			query.ExecuteInsert();
		}
	}

	return true;
}


void cNaviServer::Clear()
{
	m_svr.Close();
}
