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
		//, m_offsetRot(-0.0743381456f, 0.0576749742f, -0.0738674253f, 0.992819607f) {}
		//, m_offsetRot(0.016f, 0.042f, -0.103f, 0.994f) {}
		//, m_offsetRot(-0.019f, 0.033f, -0.103f, 0.992f) 
		//, m_offsetRot(-0.114f, 0.253f, -0.058f, 0.957f)
		//, m_offsetRot(0.318f, 0.66f, -0.476f, 0.815f)
		//, m_offsetRot(0.439f, 0.052f, -0.664f, 0.889f)
		//, m_offsetRot(0.461f, 0.065f, -0.655f, 0.884f)
		, m_offsetRot(0.319f, -0.712f, -0.481f, -0.401f)
		, m_offsetLonLat(90.f, 0.f)
		, m_angleY(180)
		, m_scaleY(1.f)
		//, m_offsetY(0.06f)
		, m_offsetY(0.184964865f)
		, m_scale(0.018f)
		, m_trackOffsetY(2.f)
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
	Vector2 m_lonLat;	// °æÀ§µµ
	gis::Vector2d m_utmLoc; // UTM ÁÂÇ¥
	cAirSpace m_airSpace;
	float m_genRouteGap = 0.0005f;
	float m_genRouteAltitude = 20.f;
	vector<gis::Vector2d> m_route;

	float m_trackOffsetY;
	vector<Vector3> m_track;
};

extern cRoot g_root;
