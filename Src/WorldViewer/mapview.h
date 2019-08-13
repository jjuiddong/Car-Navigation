//
// 2017-12-12, jjuiddong
// Map View
//
#pragma once


class cMapView : public framework::cDockWindow
{
public:
	cMapView(const string &name);
	virtual ~cMapView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnPreRender(const float deltaSeconds) override;
	virtual void OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect) override;
	virtual void OnEventProc(const sf::Event &evt) override;
	virtual void OnResetDevice() override;


protected:
	void UpdateGPS(const float deltaSeconds);
	void UpdateMapScanning(const float deltaSeconds);
	void UpdateMapTrace(const float deltaSeconds);
	void UpdateCameraTraceLookat(const bool isUpdateDistance = true);
	void UpdateLookAt();
	void OnWheelMove(const float delta, const POINT mousePt);
	void OnMouseMove(const POINT mousePt);
	void OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt);
	void OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt);
	void OnGestured(const int id, const POINT mousePt);


public:
	graphic::cSkyBoxCube m_skybox;
	graphic::cRenderTarget m_renderTarget;
	cTerrainQuadTree m_quadTree;

	bool m_showWireframe;
	float m_renderOverhead[5][500];
	int m_graphIdx;

	graphic::cSphere m_curPosObj;
	graphic::cSphere m_landMarkObj;
	graphic::cSphere m_landMarkObj2;
	gis::sGPRMC m_gpsInfo;
	Vector2d m_curGpsPos;
	float m_lookAtDistance;
	float m_lookAtYVector;
	Vector3 m_avrDir;
	bool m_isGestureInput;
	float m_gpsUpdateDelta;

	// MouseMove Variable
	POINT m_viewPos;
	sRectf m_viewRect; // detect mouse event area
	POINT m_mousePos; // window 2d mouse pos
	Vector3 m_mousePickPos; // mouse cursor pos in ground picking
	bool m_mouseDown[3]; // Left, Right, Middle
	float m_rotateLen;
	Plane m_groundPlane1;
	Plane m_groundPlane2;
};

