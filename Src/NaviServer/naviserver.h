//
// 2019-12-07, jjuiddong
// navigation server
//
#pragma once


class cNaviServer : public gps::c2s_ProtocolHandler
{
public:
	cNaviServer();
	virtual ~cNaviServer();

	bool Init(network2::cNetController &netController);
	bool Update(const float deltaSeconds);
	void Clear();


protected:
	// gps protocol handler
	virtual bool GPSInfo(gps::GPSInfo_Packet &packet) override;
	double WGS84Distance(const Vector2d &lonLat0, const Vector2d &lonLat1);


public:
	network2::cTcpServer m_svr;
	MySQLConnection m_sqlCon;
	StrPath m_pathFilename;
	string m_dateStr;
	uint64 m_journeyTimeId;
	Vector2d m_prevGpsPos;
	double m_totalDistance;
};

