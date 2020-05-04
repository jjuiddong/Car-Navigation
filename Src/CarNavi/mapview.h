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

	void ChangeViewCamera(const eCameraType camType);


protected:
	void RenderRPMGuage(const ImVec2 &pos, const float guageH, const float deltaSeconds);
	void RenderGraph(const ImVec2 &pos);
	void UpdateGPS(const float deltaSeconds);
	void UpdateOBD2(const float deltaSeconds);
	void UpdateMapScanning(const float deltaSeconds);
	void UpdateMapTrace(const float deltaSeconds);
	void MoveCamera(const Vector3 &newPos, const float camSpeed, const bool isPredict=false);

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

	// GuageBar LED
	graphic::cTexture *m_ledTexture[4]; // blue-green-yellow-red
	float m_ledBlinkTime;
	float m_ledSize;
	float m_ledAniTailR;
	float m_ledAniR;
	int m_maxRPM;
	int m_blinkRPM;
	int m_updateOBD2Period; // hz unit
	//

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
	Vector3 m_curDir;
	bool m_isGestureInput;
	float m_gpsUpdateDelta;

	// OBD2 connection
	float m_obd2ConnectTime;
	int m_obd2RcvFps;
	int m_obd2RcvCnt;
	float m_obd2RcvT;
	int m_obd2Port;

	// Navigation Client
	network2::cNetController m_netController;
	network2::cTcpClient m_naviClient;
	gps::c2s_Protocol m_gpsProtocol;
	string m_naviServerIp;
	int m_naviServerPort;
	int m_sendGpsInfo;

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

