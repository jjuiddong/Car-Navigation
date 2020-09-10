//
// 2020-06-17, jjuiddong
// Database Interface
//
#pragma once

interface iDBInterface
{

	// connect database
	virtual bool Connect(const string &dbIp, const int dbPort
		, const string &dbId, const string &dbPasswd
		, const string &dbName) = 0;

	// insert journey_date
	virtual bool InsertJourney(const string &date, const int userId
		, const uint64 timeId, const float distance, const float journeyTime) = 0;

	// update journey_date
	virtual bool UpdateJourney(const int userId, const uint64 timeId
		, const float totalDistance) = 0;

	// insert path
	virtual bool InsertPath(const string &date, const int userId
		, const uint64 timeId, const double lon, const double lat
		, const float speed, const float altitude) = 0;

	// insert landmark
	virtual bool InsertLandmark(const string &date, const int userId
		, const double lon, const double lat) = 0;

};
