//
// 2019-06-08, jjuiddong
// gps client
//
#pragma once


class cGpsClient
{
public:
	cGpsClient();
	virtual ~cGpsClient();

	bool ConnectGpsServer(const Str16 &ip, const int port);
	bool ReadPathFile(const char *fileName);
	bool GetGpsInfo(OUT gis::sGPRMC &out);
	bool IsConnect();
	bool IsReadyConnect();
	void Clear();


protected:
	bool ParseStr(const Str512 &str, OUT gis::sGPRMC &out);


public:
	enum class eState {
		Server, PathFile, GpsFile
	};

	eState m_state;
	cTimer m_timer;
	Str16 m_ip;
	int m_port;
	network2::cTcpClient m_client;
	Str512 m_recvStr;
	int m_recvCount;
	double m_recvTime;

	// path pump
	struct sPath {
		uint64 t;
		Vector2d lonLat;
	};
	vector<sPath> m_paths;	
};
