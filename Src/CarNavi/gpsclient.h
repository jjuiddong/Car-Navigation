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

	bool Init();
	bool ConnectGpsServer(const Str16 &ip, const int port);
	bool ConnectGpsSerial(const int portNum, const int baudRate);
	bool ReadPathFile(const char *fileName);
	bool GetGpsInfo(OUT gis::sGPRMC &out);
	bool GetGpsInfoFromPathFile(OUT gis::sGPRMC &out);
	bool GetGpsInfoFromGpsFile(OUT gis::sGPRMC &out);
	bool GpsReplay();
	bool PathFileReplay();
	bool StopPathFileReplay();
	bool IsConnect();
	bool IsGpsReplay();
	bool IsPathReplay();
	bool IsServer();
	bool IsReadyConnect();
	void Clear();


protected:
	bool ParseStr(const Str512 &str, OUT gis::sGPRMC &out);
	bool LogGpsFile(const char* str);


public:
	enum class eInputType {
		Network, Serial, GpsFile, PathFile
	};

	eInputType m_inputType;
	cTimer m_timer;
	StrPath m_gpsFileName; //yyyy-mm-dd

	// network
	Str16 m_ip;
	int m_port;
	network2::cTcpClient m_client;
	common::cSerialAsync m_serial;
	std::ifstream m_gpsFs; // gps file input stream

	Str512 m_recvStr;
	int m_recvCount;
	double m_recvTime;
	double m_speedRecvTime; // GPRMC에서 받은 정보를 GPATT에 적용하기위해 쓰임
	float m_speed;
	float m_altitude;

	// path pump
	struct sPath {
		uint64 t; // yyyymmddhhmmssmmm
		Vector2d lonLat;
		float speed;
	};
	vector<sPath> m_paths;
	vector<sPath> m_pathReplayData;
	int m_fileAnimationIdx;
	gis::sGPRMC m_gpsInfo;
	gis::sGPRMC m_prevGpsInfo;
	common::cDateTime2 m_prevDateTime;
	common::cDateTime2 m_prevGpsDateTime;
};
