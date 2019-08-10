
#include "stdafx.h"
#include "global.h"


cGlobal::cGlobal()
	: m_isMapScanning(false)
	, m_isSelectMapScanningCenter(false)
	, m_isShowGPS(true)
	, m_isTraceGPSPos(true)
	, m_isShowTerrain(true)
	, m_isShowRenderGraph(false)
	, m_isCalcRenderGraph(false)
	, m_analysisType(eAnalysisType::MapView)
	, m_renderT0(0.f)
	, m_renderT1(0.f)
	, m_renderT2(0.f)
	, m_isShowMapView(true)
	, m_scanRadius(1.f)
	, m_scanHeight(10.f)
	, m_scanSpeed(1.f)
	, m_isMakeTracePath(false)
{
}

cGlobal::~cGlobal()
{
	m_config.Write("carnavi_config.txt");
}


bool cGlobal::Init(HWND hwnd)
{
	m_config.Read("carnavi_config.txt");

	m_timer.Create();
	m_touch.Init(hwnd);
	m_gpsClient.Init();

	return true;
}
