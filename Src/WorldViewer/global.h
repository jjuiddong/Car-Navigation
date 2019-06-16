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


class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();


public:
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
	bool m_isMoveRight;
	bool m_pickScanLeftTop;
	bool m_pickScanRightBottom;
	Vector2d m_scanLeftTop;
	Vector2d m_scanRightBottom;
	float m_scanSpeed; // m/s
	float m_scanLineOffset; // meter

	// analysis
	eAnalysisType m_analysisType; //0:mapview, 1:terrain renderer
	double m_renderT0;
	double m_renderT1;
	double m_renderT2;
	bool m_isShowMapView;

	cTimer m_timer;
};
