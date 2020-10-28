
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

	if (!m_db.Init(cDBMgr::Database::MySql))
		return false;

	// connect db server
	const string dbIp = config.GetString("db_ip", "127.0.0.1");
	const int dbPort = config.GetInt("db_port", 10002);
	const string dbId = config.GetString("db_id", "root");
	const string dbPasswd = config.GetString("db_passwd", "root");
	const string dbName = config.GetString("db_name", "db");
	return m_db.Connect(dbIp, dbPort, dbId, dbPasswd, dbName);
}


bool cNaviServer::Update(const float deltaSeconds)
{
	//static float dt = 0.f;
	//dt += deltaSeconds;
	//if (dt > 3.f) {
	//	dt = 0.f;

	//	const cDateTime2 curDateTime = cDateTime2::Now();
	//	const Str32 strDateTime = curDateTime.GetTimeStr3(); // yyyy-mm-dd hh:mm:ss
	//	const int userId = 1;
	//	m_journeyTimeId = curDateTime.GetTimeInt64();
	//	m_db.InsertJourney(strDateTime.c_str(), userId, m_journeyTimeId, 0, 0);
	//}
	return true;
}


// gps protocol handler
bool cNaviServer::GPSInfo(gps::GPSInfo_Packet &packet)
{
	const cDateTime2 curDateTime = cDateTime2::Now();
	const Str32 dateStr = curDateTime.GetTimeStr5(); // yyyymmdd
	const int userId = 1;

	if (m_dateStr != dateStr)
	{
		const Str32 strDateTime = curDateTime.GetTimeStr4(); // yyyy-mm-dd

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
		m_db.InsertJourney(strDateTime.c_str(), userId, m_journeyTimeId, 0, 0);
	}

	const Vector2d lonLat(packet.lon, packet.lat);
	if (m_prevGpsPos != lonLat)
	{
		const Str32 strDateTime = curDateTime.GetTimeStr3(); // yyyy-mm-dd hh:mm:ss

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
		m_db.InsertPath(strDateTime.c_str(), userId, m_journeyTimeId
			, packet.lon, packet.lat, packet.speed, packet.altitude);

		// upload db journey_date
		m_db.UpdateJourney(userId, m_journeyTimeId, (float)m_totalDistance);
	}

	return true;
}


// add landmark
bool cNaviServer::AddLandMark(gps::AddLandMark_Packet &packet)
{
	const int userId = 1;
	const string dateStr = common::GetCurrentDateTime5(); // yyyy-mm-dd hh:mm:ss
	m_db.InsertLandmark(dateStr.c_str(), userId, packet.lon, packet.lat);
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
