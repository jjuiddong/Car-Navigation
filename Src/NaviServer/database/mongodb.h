//
// 2020-06-17, jjuiddong
// MongoDB data insert, update manager
//
#pragma once

#include <mongocxx/instance.hpp>

class cMongoDB : public iDBInterface
{
public:
	cMongoDB();
	virtual ~cMongoDB();

	// Override iDBInterface
	virtual bool Connect(const string &dbIp, const int dbPort
		, const string &dbId, const string &dbPasswd
		, const string &dbName) override;

	virtual bool InsertJourney(const string &date, const int userId
		, const uint64 timeId, const float distance, const float journeyTime) override;

	virtual bool UpdateJourney(const int userId, const uint64 timeId
		, const float totalDistance) override;

	virtual bool InsertPath(const string &date, const int userId
		, const uint64 timeId, const double lon, const double lat
		, const float speed, const float altitude) override;

	virtual bool InsertLandmark(const string &date, const int userId
		, const double lon, const double lat) override;

	void Clear();


public:
	mongocxx::instance *m_inst;
};
