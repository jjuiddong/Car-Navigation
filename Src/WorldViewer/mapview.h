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
	void UpdateGPS();
	void UpdateMapScanning(const float deltaSeconds);
	void UpdateLookAt();
	void OnWheelMove(const float delta, const POINT mousePt);
	void OnMouseMove(const POINT mousePt);
	void OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt);
	void OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt);
	void OnGestured(const int id, const POINT mousePt);


public:
	struct eState {
		enum Enum {
			NORMAL, PICK_RANGE,
		};
	};

	eState::Enum m_state;
	graphic::cSkyBoxCube m_skybox;
	graphic::cGridLine m_ground;
	graphic::cRenderTarget m_renderTarget;
	vector<Vector2d> m_pickRange;

	bool m_showGround;
	bool m_showWireframe;
	cTerrainQuadTree m_quadTree;
	
	Str512 m_gpsStr;
	graphic::cSphere m_curPosObj;
	Vector2d m_curGpsPos;
	float m_lookAtDistance;
	float m_lookAtYVector;
	Vector3 m_avrDir;

	bool m_isGestureInput;

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

