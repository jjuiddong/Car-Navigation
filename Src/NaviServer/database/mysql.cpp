
#include "stdafx.h"
#include "mysql.h"


cMySql::cMySql()
{
}

cMySql::~cMySql()
{
	Clear();
}


// connect mysql database
bool cMySql::Connect(const string &dbIp, const int dbPort
	, const string &dbId, const string &dbPasswd
	, const string &dbName)
{
	if (m_sqlCon.IsConnected())
		m_sqlCon.Disconnect();

	if (!m_sqlCon.Connect(dbIp, dbPort, dbId, dbPasswd, dbName))
	{
		std::cout << "DB Connection Error\n";
		return false;
	}
	return true;
}


// insert journey_date
// date : yyyy-mm-dd hh:mm:ss
bool cMySql::InsertJourney(const string &date, const int userId
	, const uint64 timeId, const float distance, const float journeyTime)
{
	RETV(!m_sqlCon.IsConnected(), false);

	const string sql =
		common::format("INSERT INTO journey_date (date, user_id, time_id"
			", distance, journey_time)"
			" VALUES ('%s', '%d', '%I64u', '%f', '%f');"
			, date.c_str(), userId, timeId, distance, journeyTime);

	MySQLQuery query(&m_sqlCon, sql);
	query.ExecuteInsert();
	return true;
}


// update journey_date
bool cMySql::UpdateJourney(const int userId, const uint64 timeId, const float totalDistance)
{
	RETV(!m_sqlCon.IsConnected(), false);

	const cDateTime2 curDateTime = cDateTime2::Now();
	const cDateTime2 date0(timeId);
	const cDateTime2 journeyTime = curDateTime - date0;

	const string sql =
		common::format("UPDATE journey_date SET journey_time = '%I64u', distance = '%f' "
			" WHERE time_id = '%I64u' AND user_id = '%d';"
			, journeyTime.m_t, totalDistance
			, timeId, userId);

	MySQLQuery query(&m_sqlCon, sql);
	query.ExecuteInsert();
	return true;
}


// insert path
// date : yyyy-mm-dd hh:mm:ss
bool cMySql::InsertPath(const string &date, const int userId
	, const uint64 timeId, const double lon, const double lat
	, const float speed, const float altitude)
{
	RETV(!m_sqlCon.IsConnected(), false);

	const string sql =
		common::format("INSERT INTO path (date_time, user_id, journey_time_id"
			", lon, lat, speed, altitude)"
			" VALUES ('%s', '%d', '%I64u', '%f', '%f', '%f', '%f');"
			, date.c_str(), userId, timeId, lon, lat
			, speed, altitude);

	MySQLQuery query(&m_sqlCon, sql);
	query.ExecuteInsert();
	return true;
}


// insert landmark
// date: yyyy-mm-dd hh:mm:ss
bool cMySql::InsertLandmark(const string &date, const int userId
	, const double lon, const double lat)
{
	RETV(!m_sqlCon.IsConnected(), false);

	const string sql =
		common::format("INSERT INTO landmark (user_id, date_time, lon, lat)"
			" VALUES ('%d', '%s', '%f', '%f');"
			, userId, date.c_str(), lon, lat);

	MySQLQuery query(&m_sqlCon, sql);
	query.ExecuteInsert();
	return true;
}


void cMySql::Clear()
{
	m_sqlCon.Disconnect();
}
