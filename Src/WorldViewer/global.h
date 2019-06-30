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

#include "touch.h"


class cMapView;
class cInformationView;
class cNavigationView;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();


public:
	// view
	cMapView *m_mapView;
	cInformationView *m_infoView;
	cNavigationView *m_naviView;

	cTouch m_touch;
	cGpsClient m_gpsClient;

	// GPS
	bool m_isShowGPS;
	bool m_isTraceGPSPos;

	// Render Overhead
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

	// path
	graphic::cDbgLineList m_lineList;

	// analysis
	eAnalysisType m_analysisType; //0:mapview, 1:terrain renderer
	double m_renderT0;
	double m_renderT1;
	double m_renderT2;
	bool m_isShowMapView;

	cTimer m_timer;
};
