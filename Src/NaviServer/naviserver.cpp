
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
	const cDateTime2 curDateTime = cDateTime2::Now();
	const Str32 dateStr = curDateTime.GetTimeStr5(); // yyyymmdd
	const Str32 strDateTime = curDateTime.GetTimeStr3(); // yyyy-mm-dd hh:mm:ss
	const int userId = 1;

	if (m_dateStr != dateStr)
	{
		// check path folder
		if (!StrPath("./path").IsFileExist())
			::CreateDirectoryA("./path", nullptr);

		m_pathFilename = "./path/path_";
		m_pathFilename += dateStr.c_str();
		m_pathFilename += ".txt";
		m_prevGpsPos = Vector2d(0, 0); // initialize
		m_totalDistance = 0.f;

		m_dateStr = dateStr;
		m_journeyTimeId = curDateTime.GetTimeInt64();

		// upload db journey_date
		if (m_sqlCon.IsConnected())
		{
			const string sql =
				common::format("INSERT INTO journey_date (date, user_id, time_id"
					", distance, journey_time)"
					" VALUES ('%s', '%d', '%I64u', '%f', '%f');"
					, strDateTime.c_str(), userId, m_journeyTimeId, 0, 0);

			MySQLQuery query(&m_sqlCon, sql);
			query.ExecuteInsert();
		}
	}

	const Vector2d lonLat(packet.lon, packet.lat);
	if (m_prevGpsPos != lonLat)
	{
		const Str32 ctime = curDateTime.GetTimeStr2();// yyyy-mm-dd-hh-mm-ss-mmm
		dbg::Logp2(m_pathFilename.c_str(), "%s, %.15f, %.15f, %f, %f\n"
			, ctime.c_str(), packet.lon, packet.lat, packet.speed, packet.altitude);

		// update distance
		if (m_prevGpsPos != Vector2d(0, 0))
		{
			const double distance = WGS84Distance(m_prevGpsPos, lonLat);
			const cDateTime2 dtime = curDateTime - m_prevDateTime;
			const double dt = ((double)dtime.m_t) * 1000.f; // delta time (second unit)
			const double speed = distance / dt;
			if (speed > 100.f) // over 300km/h?
				return false; // error occurred

			m_totalDistance += distance;
		}

		m_prevGpsPos = lonLat;
		m_prevDateTime = curDateTime;

		// filter lon/lat (korea)
		if ((lonLat.x < 123.f) || (lonLat.x > 133.f)
			|| (lonLat.y < 31.f) || (lonLat.y > 39.f))
		{
			// lon/lat position crack
			return true;
		}

		// upload db path
		if (m_sqlCon.IsConnected())
		{
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

		// upload db journey_date
		if (m_sqlCon.IsConnected())
		{
			const cDateTime2 date0(m_journeyTimeId);
			const cDateTime2 journeyTime = curDateTime - date0;

			const string sql =
				common::format("UPDATE journey_date SET journey_time = '%I64u', distance = '%f' "
					" WHERE time_id = '%I64u' AND user_id = '%d';"
					, journeyTime.m_t, m_totalDistance
					, m_journeyTimeId, userId);

			MySQLQuery query(&m_sqlCon, sql);
			query.ExecuteInsert();
		}
	}

	return true;
}


// add landmark
bool cNaviServer::AddLandMark(gps::AddLandMark_Packet &packet)
{
	if (!m_sqlCon.IsConnected())
		return true; // not connect db

	const int userId = 1;
	const string dateStr = common::GetCurrentDateTime5(); // yyyy-mm-dd hh:mm:ss

	const string sql =
		common::format("INSERT INTO landmark (user_id, date_time, lon, lat)"
			" VALUES ('%d', '%s', '%f', '%f');"
			, userId, dateStr.c_str(), packet.lon, packet.lat);

	MySQLQuery query(&m_sqlCon, sql);
	query.ExecuteInsert();

	return true;
}


// return distance lonLat0 - lonLat2 (meter unit)
// http://www.movable-type.co.uk/scripts/latlong.html
double cNaviServer::WGS84Distance(const Vector2d &lonLat0, const Vector2d &lonLat1)
{
	const double r = 6371000.f;
	const double lat0 = ANGLE2RAD2(lonLat0.y);
	const double lat1 = ANGLE2RAD2(lonLat1.y);
	const double dlat = ANGLE2RAD2(abs(lonLat0.y - lonLat1.y));
	const double dlon = ANGLE2RAD2(abs(lonLat0.x - lonLat1.x));

	const double a = sin(dlat / 2.f) * sin(dlat / 2.f)
		+ cos(lat0) * cos(lat1) * sin(dlon / 2.f) * sin(dlon / 2.f);
	const double c = 2 * atan2(sqrt(a), sqrt(1.f - a));
	const double d = r * c;
	return d;
}


void cNaviServer::Clear()
{
	m_svr.Close();
}
