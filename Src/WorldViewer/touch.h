//
// 2019-06-15, jjuiddong
// manage tablet touch, gesture, notebook input event
//
#pragma once


enum class eTouchType : int {
	Touch // touch event 
	, Gesture // gesture event
	, Mouse // pc input event
};


class cTouch
{
public:
	cTouch();
	virtual ~cTouch();

	bool Init(const HWND hwnd);
	bool IsTouchMode();
	bool IsTouchMode(const HWND hwnd);
	bool IsGestureMode();
	bool IsGestureMode(const HWND hwnd);
	eTouchType CheckTouchType(const HWND hwnd);
	bool SetTouchMode(const HWND hwnd);


public:
	eTouchType m_type;
};
