
#include "stdafx.h"
#include "mongodb.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>


cMongoDB::cMongoDB()
{
}

cMongoDB::~cMongoDB()
{
	Clear();
}


// connect mysql database
bool cMongoDB::Connect(const string &dbIp, const int dbPort
	, const string &dbId, const string &dbPasswd
	, const string &dbName)
{
	if (!m_inst)
		m_inst = new mongocxx::instance();

	return true;
}


// insert journey_date
// date : yyyy-mm-dd hh:mm:ss
bool cMongoDB::InsertJourney(const string &date, const int userId
	, const uint64 timeId, const float distance, const float journeyTime)
{
	mongocxx::client conn{ mongocxx::uri{} };
	auto collection = conn["navi"]["journeys"];

	Str128 str;
	bsoncxx::builder::stream::document document{};
	document << "date" << Str32(date).utf8().c_str();
	document << "user_id" << str.Format("%d", userId).utf8().c_str();
	document << "time_id" << str.Format("%I64u", timeId).utf8().c_str();
	document << "distance" << str.Format("%f", distance).utf8().c_str();
	document << "journey_time" << str.Format("%f", journeyTime).utf8().c_str();
	collection.insert_one(document.view());
	return true;
}


// update journey_date
bool cMongoDB::UpdateJourney(const int userId, const uint64 timeId, const float totalDistance)
{
	mongocxx::client conn{ mongocxx::uri{} };
	auto collection = conn["navi"]["journeys"];

	const cDateTime2 curDateTime = cDateTime2::Now();
	const cDateTime2 date0(timeId);
	const cDateTime2 journeyTime = curDateTime - date0;

	Str128 str;
	bsoncxx::builder::stream::document filter{};
	filter << "user_id" << str.Format("%d", userId).utf8().c_str();
	filter << "time_id" << str.Format("%I64u", timeId).utf8().c_str();

	bsoncxx::builder::stream::document document{};
	document << "$set" << bsoncxx::builder::stream::open_document;

	document << "distance" << str.Format("%f", totalDistance).utf8().c_str();
	document << "journey_time" << str.Format("%I64u", journeyTime.m_t).utf8().c_str();
	document << bsoncxx::builder::stream::close_document;

	collection.update_one(filter.view(), document.view());
	return true;
}


// insert path
// date : yyyy-mm-dd hh:mm:ss
bool cMongoDB::InsertPath(const string &date, const int userId
	, const uint64 timeId, const double lon, const double lat
	, const float speed, const float altitude)
{
	mongocxx::client conn{ mongocxx::uri{} };
	auto collection = conn["navi"]["paths"];

	Str128 str;
	bsoncxx::builder::stream::document document{};
	document << "date_time" << Str32(date).utf8().c_str();
	document << "user_id" << str.Format("%d", userId).utf8().c_str();
	document << "journey_time_id" << str.Format("%I64u", timeId).utf8().c_str();
	document << "lon" << str.Format("%f", lon).utf8().c_str();
	document << "lat" << str.Format("%f", lat).utf8().c_str();
	document << "speed" << str.Format("%f", speed).utf8().c_str();
	document << "altitude" << str.Format("%f", altitude).utf8().c_str();
	collection.insert_one(document.view());
	return true;
}


// insert landmark
// date: yyyy-mm-dd hh:mm:ss
bool cMongoDB::InsertLandmark(const string &date, const int userId
	, const double lon, const double lat)
{
	mongocxx::client conn{ mongocxx::uri{} };
	auto collection = conn["navi"]["landmarks"];

	Str128 str;
	bsoncxx::builder::stream::document document{};
	document << "date_time" << Str32(date).utf8().c_str();
	document << "user_id" << str.Format("%d", userId).utf8().c_str();
	document << "lon" << str.Format("%f", lon).utf8().c_str();
	document << "lat" << str.Format("%f", lat).utf8().c_str();
	document << "address" << str.Format("").utf8().c_str();
	document << "comment" << str.Format("").utf8().c_str();
	collection.insert_one(document.view());
	return true;
}


void cMongoDB::Clear()
{
	SAFE_DELETE(m_inst);
}
