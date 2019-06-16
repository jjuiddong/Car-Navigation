
#include "stdafx.h"
#include "global.h"


cGlobal::cGlobal()
	: m_isMapScanning(false)
	, m_isMoveRight(false)
	, m_pickScanLeftTop(false)
	, m_pickScanRightBottom(false)
	, m_scanLeftTop(125.646400f, 38.00f)
	, m_scanRightBottom(129.657455f, 34.685825f)
	, m_scanSpeed(10.f)
	, m_scanLineOffset(30.f)
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
{
	m_timer.Create();
}

cGlobal::~cGlobal()
{
}

