
#include "stdafx.h"
#include "touch.h"


cTouch::cTouch()
	: m_type(eTouchType::Mouse)
{
}

cTouch::~cTouch()
{
}


// 초기화 될 때, 하드웨어 스펙을 검사해서 touch 이벤트가 가능한지 확인한다.
// touch 이벤트가 안되면 Mouse 타입이 된다.
// https://docs.microsoft.com/en-us/windows/desktop/wintouch/getting-started-with-multi-touch-messages
bool cTouch::Init(const HWND hwnd)
{
	const int value = GetSystemMetrics(SM_DIGITIZER);
	if (value & NID_READY) 
	{ 
		//stack ready
		m_type = eTouchType::Touch;
	}

	if (value  & NID_MULTI_INPUT) 
	{ 
		//digitizer is multitouch
		m_type = eTouchType::Touch;
	}

	if (value & NID_INTEGRATED_TOUCH) 
	{ 
		// Integrated touch
		m_type = eTouchType::Touch;
		SetTouchMode(hwnd);
	}

	if (0 == value)
	{
		m_type = eTouchType::Mouse;
	}

	return true;
}


bool cTouch::IsTouchMode()
{
	return (eTouchType::Touch == m_type);
}


bool cTouch::IsTouchMode(const HWND hwnd)
{
	if (eTouchType::Mouse == m_type)
		return false;
	return IsTouchWindow(hwnd, NULL);
}


bool cTouch::IsGestureMode()
{
	return (eTouchType::Gesture == m_type);
}


bool cTouch::IsGestureMode(const HWND hwnd)
{
	if (eTouchType::Mouse == m_type)
		return false;
	return !IsTouchWindow(hwnd, NULL);
}


eTouchType cTouch::CheckTouchType(const HWND hwnd)
{
	if (eTouchType::Mouse == m_type)
		return eTouchType::Mouse;

	m_type = IsTouchWindow(hwnd, NULL) ? eTouchType::Touch : eTouchType::Gesture;
	return m_type;
}


bool cTouch::SetTouchMode(const HWND hwnd)
{
	if (eTouchType::Mouse == m_type)
		return false;
	return RegisterTouchWindow(hwnd, 0) ? true : false;
}


bool cTouch::SetGestureMode(const HWND hwnd)
{
	if (eTouchType::Mouse == m_type)
		return false;
	UnregisterTouchWindow(hwnd);
	return true;
}
