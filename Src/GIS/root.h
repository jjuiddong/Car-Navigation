//
// 2018-05-08, jjuiddong
// Global Root class
//
#pragma once


class cMapView;

class cRoot
{
public:
	cRoot() : m_renderFlag(0x01 | 0x02)
		, m_offsetRot(0.319f, -0.712f, -0.481f, -0.401f)
		, m_offsetLonLat(90.f, 0.f)
		, m_angleY(180)
		, m_scaleY(1.f)
		, m_offsetY(0.184964865f)
		, m_scale(0.018f)
	{}
	virtual ~cRoot() {}


public:
	int m_renderFlag;
	cMapView *m_mapView;

	// facility position
	Vector2 m_offsetLonLat;
	float m_angleY;

	Quaternion m_offsetRot;
	float m_scaleY;
	float m_offsetY;
	float m_scale;

	// lat,lon position
	Vector2 m_lonLat; // °æÀ§µµ
	Vector2d m_utmLoc; // UTM ÁÂÇ¥
	float m_genRouteGap = 0.0005f;
	float m_genRouteAltitude = 20.f;
	vector<Vector2d> m_route;
};

extern cRoot g_root;
