//
// 2020-06-17, jjuiddong
// database manager
//	- MySQL, MongoDB Switching
//
#pragma once


class cDBMgr : public iDBInterface
{
public:
	enum class Database { MySql, MongoDB};

	cDBMgr();
	virtual ~cDBMgr();

	bool Init(const Database db);
	void Clear();

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


public:
	Database m_dbType;
	iDBInterface *m_db;
};
