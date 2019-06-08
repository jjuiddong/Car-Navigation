
#include "stdafx.h"
#include "global.h"


cGlobal::cGlobal()
	: m_gpsClient(new network2::cPacketHeaderNoFormat())
	, m_isMapScanning(false)
	, m_isMoveRight(false)
	, m_pickScanLeftTop(false)
	, m_pickScanRightBottom(false)
	, m_scanLeftTop(125.646400f, 38.00f)
	, m_scanRightBottom(129.657455f, 34.685825f)
	, m_scanSpeed(10.f)
	, m_scanLineOffset(30.f)
{
	m_timer.Create();
}

cGlobal::~cGlobal()
{
}

