
#include "stdafx.h"
#include "dbmgr.h"


cDBMgr::cDBMgr()
	: m_db(nullptr)
{
}

cDBMgr::~cDBMgr()
{
	Clear();
}


// initialize database manager
// set database mysql or mongodb
bool cDBMgr::Init(const Database db)
{
	Clear();

	switch (db)
	{
	case Database::MongoDB: m_db = new cMongoDB(); break;
	case Database::MySql: m_db = new cMySql(); break;
	default: assert(0); break;
	}

	return true;
}


void cDBMgr::Clear()
{
	SAFE_DELETE(m_db);
}


bool cDBMgr::Connect(const string &dbIp, const int dbPort
	, const string &dbId, const string &dbPasswd
	, const string &dbName)
{
	RETV(!m_db, false);
	return m_db->Connect(dbIp, dbPort, dbId, dbPasswd, dbName);
}


bool cDBMgr::InsertJourney(const string &date, const int userId
	, const uint64 timeId, const float distance, const float journeyTime)
{
	RETV(!m_db, false);
	return m_db->InsertJourney(date, userId, timeId, distance, journeyTime);
}


bool cDBMgr::UpdateJourney(const int userId, const uint64 timeId
	, const float totalDistance)
{
	RETV(!m_db, false);
	return m_db->UpdateJourney(userId, timeId, totalDistance);
}


bool cDBMgr::InsertPath(const string &date, const int userId
	, const uint64 timeId, const double lon, const double lat
	, const float speed, const float altitude)
{
	RETV(!m_db, false);
	return m_db->InsertPath(date, userId, timeId, lon, lat, speed, altitude);
}


bool cDBMgr::InsertLandmark(const string &date, const int userId
	, const double lon, const double lat)
{
	RETV(!m_db, false);
	return m_db->InsertLandmark(date, userId, lon, lat);
}
