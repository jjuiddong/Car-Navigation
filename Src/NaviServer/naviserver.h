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


public:
	network2::cTcpServer m_svr;
	MySQLConnection m_sqlCon;
	StrPath m_pathFilename;
	string m_dateStr;
	unsigned __int64 m_journeyTimeId;
	Vector2d m_prevGpsPos;
};

