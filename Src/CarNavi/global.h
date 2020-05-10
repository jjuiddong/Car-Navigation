//
// 2019-05-26, jjuiddong
// global variable
//
#pragma once

enum class eAnalysisType : int 
{
	MapView
	, Terrain
	, GMain
};


// trace camera view type
enum class eCameraType : int
{
	Custom
	, Camera1 // first person view
	, Camera2
	, Camera3
	, Camera4
	, MAX
};

struct sCameraInfo {
	float lookAtY;
	float distance;
};



class cMapView;
class cInformationView;
class cNavigationView;

class cGlobal : public iOBD2Receiver
{
public:
	cGlobal();
	virtual ~cGlobal();
	
	bool Init(HWND hwnd);
	bool ReadPathFiles(graphic::cRenderer &renderer, cTerrainQuadTree &terrain
		, const StrPath pathDirectoryName);
	bool Read3DPosFiles(graphic::cRenderer &renderer, const StrPath pathDirectoryName);
	bool ReadAndConvertPathFiles(graphic::cRenderer &renderer, cTerrainQuadTree &terrain
		, const StrPath &pathDirectoryName);
	bool ConvertTrackPos2Path();
	void Clear();


protected:
	virtual void Recv(const int pid, const int data) override;


public:
	// view
	cMapView *m_mapView;
	cInformationView *m_infoView;
	cNavigationView *m_naviView;

	cTouch m_touch;
	cGpsClient m_gpsClient;
	cOBD2 m_obd;
	cConfig m_config;
	
	// GPS
	bool m_isShowGPS;
	bool m_isTraceGPSPos;
	bool m_isRotateTrace;

	// ODB2
	int m_rpm;
	int m_speed; // km/s
	int m_gear; // transimission gear
	int m_obdRcvCnt;

	// View Type
	eCameraType m_camType;
	sCameraInfo m_camInfo[(int)eCameraType::MAX];

	// Render Overlay
	bool m_isShowTerrain;
	bool m_isShowRenderGraph;
	bool m_isCalcRenderGraph;

	// map scanning
	bool m_isMapScanning; // 카메라를 정해진 경로로 움직일 때 true
	bool m_isSelectMapScanningCenter; // 스캐닝 할 중점을 선택한다.
	Vector2d m_scanCenter;
	Vector3 m_scanCenterPos;
	Vector3 m_scanPos;
	Vector3 m_scanDir;
	float m_scanRadius;
	float m_scanHeight;
	float m_scanSpeed; // m/s

	// map trace
	bool m_isMakeTracePath;
	Vector3 m_prevTracePos;

	// path
	bool m_isShowPrevPath;
	StrPath m_pathFilename;
	vector<cPathRenderer*> m_pathRenderers;

	// landmark
	bool m_isShowLandMark;
	int m_landMarkSelectState; //0=none, 1=ready, 2=set
	cLandMark m_landMark;

	// landmark2, temporal landmark
	bool m_isShowLandMark2;
	int m_landMarkSelectState2; //0=none, 1=ready, 2=set
	cLandMark m_landMark2;

	// analysis
	eAnalysisType m_analysisType; //0:mapview, 1:terrain renderer
	double m_renderT0;
	double m_renderT1;
	double m_renderT2;
	bool m_isShowMapView;

	// option
	bool m_isDebugMode;
	bool m_isDarkMode; // screen dark mode
	Vector4 m_darkColor;

	cShapefileLoader m_shape;

	cTimer m_timer;
};
