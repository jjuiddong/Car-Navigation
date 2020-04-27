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
	bool GetGpsInfoFromFile(OUT gis::sGPRMC &out);
	bool GpsReplay();
	bool FileReplay();
	bool StopFileReplay();
	bool IsConnect();
	bool IsGpsReplay();
	bool IsPathReplay();
	bool IsServer();
	bool IsReadyConnect();
	void Clear();


protected:
	bool ParseStr(const Str512 &str, OUT gis::sGPRMC &out);


public:
	enum class eInputType {
		Network, Serial, GpsFile, PathFile
	};

	eInputType m_inputType;
	cTimer m_timer;

	// network
	Str16 m_ip;
	int m_port;
	network2::cTcpClient m_client;
	// serial
	common::cSerialAsync m_serial;
	// gps file
	std::ifstream m_gpsFs; // gps file input stream

	Str512 m_recvStr;
	int m_recvCount;
	double m_recvTime;
	double m_speedRecvTime; // GPRMC에서 받은 정보를 GPATT에 적용하기위해 쓰임
	float m_speed;

	// path pump
	struct sPath {
		uint64 t;
		Vector2d lonLat;
	};
	vector<sPath> m_paths;
	int m_fileAnimationIdx;
};
