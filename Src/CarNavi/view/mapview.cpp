
#include "stdafx.h"
#include "mapview.h"
#include "navigationview.h"
#include "../optimize/pointmapper.h"
#include "../optimize/qtreegraph.h"

using namespace graphic;
using namespace framework;


cMapView::cMapView(const string &name) 
	: framework::cDockWindow(name)
	, m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_groundPlane2(Vector3(0, -1, 0), 0)
	, m_showWireframe(false)
	, m_lookAtDistance(0)
	, m_lookAtYVector(0)
	, m_graphIdx(0)
	, m_gpsUpdateDelta(0)
	, m_obd2ConnectTime(0)
	, m_ledBlinkTime(0.f)
	, m_ledSize(30.f)
	, m_maxRPM(2200)
	, m_blinkRPM(2200)
	, m_updateOBD2Period(10)
	, m_ledAniTailR(0.7f)
	, m_ledAniR(0.1f)
	, m_obd2RcvT(0.f)
	, m_obd2RcvFps(0)
	, m_obd2Port(4)
	, m_naviServerIp("220.77.25.119")
	, m_naviServerPort(10001)
	, m_sendGpsInfo(0)
{
	ZeroMemory(m_renderOverhead, sizeof(m_renderOverhead));
}

cMapView::~cMapView() 
{
	m_quadTree.Clear();
	m_netController.Clear();
	m_naviClient.Close();
}


bool cMapView::Init(cRenderer &renderer) 
{
	const Vector3 eyePos(3040.59766f, 10149.6260f, -4347.90381f);
	const Vector3 lookAt(2825.30078f, 0.000000000f, 2250.73193f);
	const float fov = g_global->m_config.GetFloat("fov", MATH_PI / 4.f);
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
	m_camera.SetProjection(fov, m_rect.Width() / m_rect.Height(), 0.1f, 100000.f);
	m_camera.SetViewPort(m_rect.Width(), m_rect.Height());
	m_camera.m_kp = g_global->m_config.GetFloat("camera_kp", 1.5f);
	m_camera.m_kd = g_global->m_config.GetFloat("camera_kd", 0.f);

	sf::Vector2u size((u_int)m_rect.Width() - 15, (u_int)m_rect.Height() - 50);
	cViewport vp = renderer.m_viewPort;
	vp.m_vp.Width = (float)size.x;
	vp.m_vp.Height = (float)size.y;
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true
		, DXGI_FORMAT_D24_UNORM_S8_UINT);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL);

	if (!m_quadTree.Create(renderer, true))
		return false;

	m_quadTree.m_isShowDistribute = false;
	m_quadTree.m_isShowQuadTree = false;
	m_quadTree.m_isShowFacility = false;
	m_quadTree.m_isShowPoi1 = true;
	m_quadTree.m_isShowPoi2 = false;
	if (m_quadTree.m_tileMgr)
		m_quadTree.m_tileMgr->m_geoDownloader.Create(
			g_global->m_config.GetString("apikey"));

	m_skybox.Create(renderer, "../media/skybox/sky.dds");
	m_skybox.m_isDepthNone = true;

	m_curPosObj.Create(renderer, 1.f, 10, 10
		, (eVertexType::POSITION | eVertexType::NORMAL), cColor::RED);
	m_landMarkObj.Create(renderer, 1.f, 10, 10
		, (eVertexType::POSITION), cColor(0.8f, 0.8f, 0.2f, 1.f));
	m_landMarkObj2.Create(renderer, 1.f, 10, 10
		, (eVertexType::POSITION), cColor(0.8f, 0.8f, 0.2f, 1.f));

	m_ledTexture[0] = cResourceManager::Get()->LoadTexture(renderer, "./media/led/blue.png");
	m_ledTexture[1] = cResourceManager::Get()->LoadTexture(renderer, "./media/led/green.png");
	m_ledTexture[2] = cResourceManager::Get()->LoadTexture(renderer, "./media/led/yellow.png");
	m_ledTexture[3] = cResourceManager::Get()->LoadTexture(renderer, "./media/led/red.png");

	m_ledSize = g_global->m_config.GetFloat("guage_led_size", 30.f);
	m_maxRPM = g_global->m_config.GetInt("max_rpm", 2200);
	m_blinkRPM = g_global->m_config.GetInt("blink_rpm", m_maxRPM);
	m_updateOBD2Period = g_global->m_config.GetInt("update_obd_period", 6);
	m_ledAniTailR = g_global->m_config.GetFloat("led_ani_tail", 0.7f);
	m_ledAniR = g_global->m_config.GetFloat("led_ani", 0.1f);
	m_obd2Port = g_global->m_config.GetInt("obd2_port", 4);
	m_naviServerIp = g_global->m_config.GetString("naviserver_ip", "220.77.251.119");
	m_naviServerPort = g_global->m_config.GetInt("naviserver_port", 10001);
	g_global->m_isDebugMode = g_global->m_config.GetBool("debug_mode", false);
	m_quadTree.m_distanceLevelOffset = g_global->m_config.GetInt("distance_lv_offset", 20);

	m_naviClient.RegisterProtocol(&m_gpsProtocol);
	m_netController.StartTcpClient(&m_naviClient, m_naviServerIp, m_naviServerPort);

	return true;
}


void cMapView::OnUpdate(const float deltaSeconds)
{
	// ī�޶� ����Ű�� ������ �������� ���Ѵ�.
	const Vector2d camLonLat = m_quadTree.GetLongLat(m_camera.GetRay());
	cGpsClient &gpsClient = g_global->m_gpsClient;

	if (g_global->m_isShowTerrain)
		m_quadTree.Update(GetRenderer(), camLonLat, deltaSeconds);

	float dt = deltaSeconds;
	// �Ǽ�.
	// FileReplay ���� ��, ����Ÿ�� ��û ���̶�� ����Ѵ�.
	// ī�޶� �̵����� �ʴ´�.
	//if (gpsClient.IsPathReplay()
	//	&& (m_quadTree.m_tileMgr->m_geoDownloader.m_requestIds.size() > 1))
	if (g_global->m_isMapScanning && (g_global->m_scanType != 1)
		&& (m_quadTree.m_tileMgr->m_geoDownloader.m_requestIds.size() > 1))
	{
		dt = 0.f;
	}

	m_camera.Update(dt);
	m_netController.Process(dt);

	g_global->m_touch.CheckTouchType(m_owner->getSystemHandle());

	UpdateOBD2(deltaSeconds);
	UpdateGPS(deltaSeconds);
	UpdateMapScanCircle(deltaSeconds);
	UpdateMapScanPath(deltaSeconds);
	//UpdateMapTrace(deltaSeconds);
}


// GPS ������ ���� ��ġ������ �޾� ������Ʈ �Ѵ�.
// receive from gps server (mobile phone)
void cMapView::UpdateGPS(const float deltaSeconds)
{
	const float MIN_LENGTH = 0.1f;
	const float MAX_LENGTH = 1.0f; // ª�� �ð��� ���̰� ū ���� ������ �����Ѵ�.
	m_gpsUpdateDelta += deltaSeconds;

	// �׺���̼� ������ ������ ����ٸ�, 5�ʰ��ݸ��� �������� �õ��Ѵ�.
	static float checkSvrConTime = 0.f;
	if (m_naviClient.IsFailConnection())
	{
		checkSvrConTime += deltaSeconds;
		if (checkSvrConTime > 5.f)
		{
			checkSvrConTime = 0.f;
			m_naviClient.ReConnect();
		}
	}

	// GPS ������ �����´�.
	gis::sGPRMC gpsInfo;
	if (!g_global->m_gpsClient.GetGpsInfo(gpsInfo))
	{
		// no gps info and camera stop moving?
		// move camera to future position
		// 6 second predict
		if (!m_camera.IsMoveAnimation())
		{
			Vector3 pos = m_quadTree.Get3DPos(m_curGpsPos);
			const float mov = gis::Meter23DUnit(m_gpsInfo.speed * 1000 * (6.f / 3600.f));
			const Vector3 diff = m_curDir * mov;
			pos += diff;
			const float moveSpeed = mov / 5.f;
			MoveCamera(pos, moveSpeed, true);
		}
		return;
	}

	m_gpsInfo = gpsInfo;
	m_curGpsPos = gpsInfo.lonLat;

	if (!g_global->m_obd.IsOpened()) // if not connect OBD2, update from gps info
		g_global->m_speed = (int)gpsInfo.speed;

	static Vector2d oldGpsPos2; // for trace gps position
	static Vector3 oldEyePos;
	if (oldGpsPos2.IsEmpty())
		oldGpsPos2 = m_curGpsPos;

	if (m_lookAtDistance == 0)
		m_lookAtDistance = min(200.f, m_camera.GetEyePos().Distance(m_camera.GetLookAt()));

	// write path log file
	if (g_global->m_gpsClient.IsConnect()
		&& !g_global->m_gpsClient.IsGpsReplay()
		&& !g_global->m_gpsClient.IsPathReplay())
	{
		const string date = common::GetCurrentDateTime();
		static Vector2d prevGpsPos;
		if (prevGpsPos != m_curGpsPos)
		{
			dbg::Logp2(g_global->m_pathFilename.c_str(), "%s, %.15f, %.15f, %f, %f\n"
				, date.c_str(), m_curGpsPos.x, m_curGpsPos.y, gpsInfo.speed, gpsInfo.altitude);
			prevGpsPos = m_curGpsPos;

			// send to Navigation Server with GPS Information
			if (m_naviClient.IsConnect())
			{
				++m_sendGpsInfo;
				m_gpsProtocol.GPSInfo(network2::SERVER_NETID
					, m_curGpsPos.x, m_curGpsPos.y, gpsInfo.altitude, gpsInfo.speed);
			}
		}
	}

	// ���� ��ġ�� ���� ī�޶� looAt�� �����Ѵ�.
	// lookAt�� �ٲ� ���� ī�޶� ��ġ�� ���� �Ÿ��� �����ϸ� �̵��Ѵ�.
	g_global->m_lonLat = Vector2((float)gpsInfo.lonLat.x, (float)gpsInfo.lonLat.y);
	const Vector3 pos = m_quadTree.Get3DPos(m_curGpsPos);
	MoveCamera(pos, 0.f);

	// save move path
	// ���� ��ġ�� ���� ���ٸ� �������� �ʴ´�.
	{
		auto &track = g_global->m_gpsClient.m_paths;
		const Vector3 lastPos = track.empty() ? 
			Vector3::Zeroes : m_quadTree.Get3DPos(track.back().lonLat);

		if (track.empty()
			|| (!track.empty() && (lastPos.Distance(pos) > MIN_LENGTH)))
			track.push_back({ common::GetCurrentDateTime3(), gpsInfo.lonLat
				, gpsInfo.speed});
	}
}


// OBD2 serial communication
void cMapView::UpdateOBD2(const float deltaSeconds)
{
	if (g_global->m_obd.IsOpened())
	{
		// calc fps
		m_obd2RcvT += deltaSeconds;
		if (m_obd2RcvT > 1.f)
		{
			const int c = g_global->m_obdRcvCnt - m_obd2RcvCnt;
			m_obd2RcvFps = c;
			m_obd2RcvCnt = g_global->m_obdRcvCnt;
			m_obd2RcvT = 0.f;
		}

		static float incT = 0;
		incT += deltaSeconds;
		const float t = 1.f / (float)m_updateOBD2Period;
		if (incT > t) // query rpm, speed data to OBD2
		{
			g_global->m_obd.Query(cOBD2::PID_RPM);
			g_global->m_obd.Query(cOBD2::PID_SPEED);
			//g_global->m_obd.Query(cOBD2::PID_TRANSMISSION_GEAR);
			incT = 0.f;
		}
		m_obd2ConnectTime = 0.f;
	}
	else
	{
		// OBD2������ ����ٸ�, 5�ʸ��� �ѹ��� ������ �õ��Ѵ�.
		m_obd2ConnectTime += deltaSeconds;
		if (m_obd2ConnectTime > 5.f)
		{
			m_obd2ConnectTime = 0.f;
			g_global->m_obd.Open(m_obd2Port, 115200, g_global); //COM4
			common::dbg::Logp("ODB2 Connect COM%d\n", m_obd2Port);
		}
	}
}


// circle scanning
void cMapView::UpdateMapScanCircle(const float deltaSeconds)
{
	if (!g_global->m_isMapScanning || (g_global->m_scanType != 0))
		return;
	if (m_quadTree.m_tileMgr->m_geoDownloader.m_requestIds.size() > 1)
		return;

	Vector3 curPos = g_global->m_scanPos;
	curPos.y = 0;

	Vector3 eyePos = curPos;
	eyePos.y = g_global->m_scanHeight;

	const Vector3 nextPos = curPos + g_global->m_scanDir * g_global->m_scanSpeed;
	const Vector3 lookAt = curPos + g_global->m_scanDir * 30.f;
	
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));

	Vector3 center = g_global->m_scanCenterPos;
	Vector3 dir = (nextPos - center);
	dir.y = 0.f;
	dir.Normalize();
	g_global->m_scanDir = Vector3(0, 1, 0).CrossProduct(dir).Normal();
	g_global->m_scanDir += dir * 0.3f * deltaSeconds; // ���ݾ� ������ ������ ư��.
	g_global->m_scanPos = nextPos;
}


// convex scanning
void cMapView::UpdateMapScanPath(const float deltaSeconds)
{
	if (!g_global->m_isMapScanning || (g_global->m_scanType != 1))
		return;
	if (m_quadTree.m_tileMgr->m_geoDownloader.m_requestIds.size() > 1)
		return;

	cTerrainQuadTree &terrain = m_quadTree;
	const float dist = g_global->m_scanPos.Distance(g_global->m_scanNextPos);
	if ((dist < 0.01f) || (dist > g_global->m_prevDistance)) {
		// arrive, next position
		const uint nidx = (g_global->m_scanPathIdx + 1) % g_global->m_scanPath.size();
		g_global->m_scanPathIdx = nidx;
		g_global->m_prevDistance = FLT_MAX;

		// 1 cycle finished
		// change scan path to more center nearby
		if (nidx == 0)
		{
			double totDist = 0.f;
			for (uint i = 0; i < g_global->m_scanPath.size(); ++i) 
			{
				const Vector2d lonLat = g_global->m_scanPath[i];
				totDist += g_global->m_scanCenter.Distance(lonLat);
				const Vector2d dir = (g_global->m_scanCenter - lonLat).Normal();
				const Vector2d pos = lonLat + dir * 0.0005f * (double)g_global->m_scanHeight;
				g_global->m_scanPath[i] = pos;
			}

			const double avrDist = totDist / (double)g_global->m_scanPath.size();
			if (avrDist < 0.05f)
			{
				// finish scan, recovery source path, height down, re scan
				g_global->m_scanPath = g_global->m_scanSrcPath;
				g_global->m_scanHeight -= 20.f;
				if (g_global->m_scanHeight < 10.f)
				{
					// finish scan
					g_global->m_isMapScanning = false;
				}
			}
		}

		MoveScanPathCamera(true);
	}
	else
	{
		g_global->m_prevDistance = dist;
	}


	const float camDist = g_global->m_scanHeight * 0.05f; // lookat distance
	Vector3 eyePos = g_global->m_scanPos + 
		g_global->m_scanDir * g_global->m_scanSpeed * deltaSeconds;
	g_global->m_scanPos = eyePos;
	Vector3 lookAt = eyePos + g_global->m_scanDir * camDist;
	eyePos.y = g_global->m_scanHeight;
	m_camera.SetLookAt(lookAt);
	m_camera.Move(eyePos, lookAt, g_global->m_scanSpeed);
}


// move camera when scan path mode
void cMapView::MoveScanPathCamera(const bool isUpdateCurPos)
{
	if (!g_global->m_isMapScanning || (g_global->m_scanType != 1))
		return;

	cTerrainQuadTree &terrain = m_quadTree;
	const vector<Vector2d> &path = g_global->m_scanPath;
	const uint idx = g_global->m_scanPathIdx;
	const float height = g_global->m_scanHeight;
	const uint nidx = (idx + 1) % path.size();
		
	if (isUpdateCurPos) 
	{
		Vector3 pos = terrain.Get3DPos(path[idx]);
		pos.y = height;
		m_camera.SetEyePos(pos);
	}

	Vector3 pos0 = m_camera.GetEyePos();
	Vector3 pos1 = terrain.Get3DPos(path[nidx]);
	pos0.y = 0;
	pos1.y = 0;
	const Vector3 dir = (pos1 - pos0).Normal();

	g_global->m_scanPos = pos0;
	g_global->m_scanNextPos = pos1;
	g_global->m_scanDir = dir;
}


// path ��η� �̵� ���� �� ó��
void cMapView::UpdateMapTrace(const float deltaSeconds)
{
	//cGpsClient &gpsClient = g_global->m_gpsClient;
	//if (!gpsClient.IsPathReplay())
	//	return;

	//// ����Ÿ�� ��û ���̶��, ��ǥ���� �̵����� �ʴ´�.
	//if (m_quadTree.m_tileMgr->m_geoDownloader.m_requestIds.size() > 1)
	//	return;

	//// ī�޶� ��ǥ ��ġ�� �̵� ���̶��, ��ǥ���� �ٲ��� �ʴ´�.
	//if (!g_global->m_prevTracePos.IsEmpty())
	//{
	//	Vector3 eyePos = m_camera.GetEyePos();
	//	eyePos.y = 0;
	//	const float dist = g_global->m_prevTracePos.Distance(eyePos);
	//	if (dist > 5.f)
	//		return;
	//}

	//gis::sGPRMC gpsInfo;
	//if (!gpsClient.GetGpsInfoFromPathFile(gpsInfo))
	//	return;

	//Vector3 pos = m_quadTree.Get3DPos(gpsInfo.lonLat);
	//pos.y = 0.f;
	//if (g_global->m_prevTracePos.IsEmpty())
	//{
	//	g_global->m_prevTracePos = pos;
	//	m_camera.SetEyePos(pos);
	//	return;
	//}

	//Vector3 p0 = g_global->m_prevTracePos;
	//p0.y = 0.f;
	//Vector3 dir = (pos - p0).Normal();
	//const float dist = pos.Distance(p0);

	//Vector3 eyePos = pos;
	//eyePos.y = g_global->m_scanHeight;
	//Vector3 lookAt = pos + dir * dist;
	//lookAt.y = pos.y;
	//m_camera.SetLookAt(lookAt);
	//m_camera.Move(eyePos, lookAt, 30.f);
	//g_global->m_prevTracePos = pos;
}


// set camera moving animation
void cMapView::MoveCamera(const Vector3 &newPos, const float camSpeed
	, const bool isPredict //=false
)
{
	const float MIN_LENGTH = 0.1f;
	const float MAX_LENGTH = 1.0f; // ª�� �ð��� ���̰� ū ���� ������ �����Ѵ�.

	static Vector2d oldGpsPos2; // for trace gps position
	static Vector3 oldEyePos;

	if (oldGpsPos2.IsEmpty())
		oldGpsPos2 = m_curGpsPos;

	const Vector3 pos = newPos;
	const Vector3 oldPos2 = m_quadTree.Get3DPos(oldGpsPos2);

	const bool isTraceGPSPos = g_global->m_isTraceGPSPos
		&& !m_mouseDown[0] && !m_mouseDown[1]
		&& (g_global->m_touch.m_type != eTouchType::Gesture);
	if (!isTraceGPSPos && (g_global->m_camType == eCameraType::Custom))
		m_lookAtYVector = 0;

	if (isTraceGPSPos && (m_lookAtYVector == 0.f))
		m_lookAtYVector = m_camera.GetDirection().y;

	// ī�޶� ������ �ٲ۴�. �̵��ϴ� �������� ���ϰ� �Ѵ�.
	// �ֱ� �̵� �������� n���� ���͸� ����ؼ� ���� ������ �����Ѵ�. (����ġ ���)
	Vector3 avrDir;
	{
		// �ֱ� n1�� 80% ����ġ
		// ������ n2�� 20% ����ġ
		const int n1 = 5;
		const int n2 = 5;
		const float r1 = 80.f / (float)n1;
		const float r2 = 20.f / (float)n2;

		int cnt = 0;
		auto &track = g_global->m_gpsClient.m_paths;
		if (isPredict) //predict mode? latest direction is avr dir
		{
			if (track.size() >= 2)
			{
				const Vector3 p0 = m_quadTree.Get3DPos(track[track.size()-2].lonLat);
				const Vector3 p1 = m_quadTree.Get3DPos(track.back().lonLat);
				avrDir = p1 - p0;
				avrDir.y = 0.f;
			}
		}
		else
		{
			for (int i = (int)track.size() - 1; (i >= 1) && (cnt < (n1 + n2)); --i, ++cnt)
			{
				const Vector2d &ll0 = track[i - 1].lonLat;
				const Vector2d &ll1 = track[i].lonLat;
				const Vector3 p0 = m_quadTree.Get3DPos(ll0);
				const Vector3 p1 = m_quadTree.Get3DPos(ll1);
				Vector3 d = p1 - p0;
				d.y = 0.f;
				d.Normalize();

				if (i == (int)track.size() - 1)
					m_curDir = d; // current direction

				if (cnt < n1)
					avrDir += d * r1;
				else
					avrDir += d * r2;
			}
		}
		avrDir.Normalize();
		m_avrDir = avrDir;
	}

	Vector3 dir = (g_global->m_isRotateTrace) ? avrDir : m_camera.GetDirection();
	dir.y = m_lookAtYVector;
	const float lookAtDis = pos.Distance(m_camera.GetEyePos());
	const float offsetY = (g_global->m_camType == eCameraType::Camera1) ?
		0.2f : max(1.f, min(8.f, (lookAtDis - 25.f) * 0.2f));
	const Vector3 lookAtPos = pos + Vector3(0, offsetY, 0);
	Vector3 newEyePos = lookAtPos + dir * -m_lookAtDistance;
	const float newY = m_quadTree.Get3DPos(m_quadTree.GetWGS84(newEyePos)).y;
	newEyePos.y = max(newEyePos.y, newY + 0.1f);

	// ª�� �ð��� ���̰� ū ���� ������ �����Ѵ�.
	const Vector3 p0(pos.x, 0, pos.z);
	const Vector3 p1(oldPos2.x, 0, oldPos2.z);
	const bool isIgnoreTrace = !isPredict && (p1.Distance(p0) > MAX_LENGTH) && (m_gpsUpdateDelta < 3.f);
	m_gpsUpdateDelta = 0.f;

	if (oldPos2.Distance(pos) > MIN_LENGTH)
	{
		// ����ó �Է� �ÿ��� ī�޶� �ڵ����� �������� �ʴ´�.
		if (isTraceGPSPos && !avrDir.IsEmpty() && !isIgnoreTrace && (m_lookAtYVector != 0))
			m_camera.Move(newEyePos, lookAtPos, camSpeed);

		if (!isIgnoreTrace)
		{
			oldEyePos = newEyePos;
		}

		oldGpsPos2 = m_curGpsPos;
	}
	else if (isTraceGPSPos && !avrDir.IsEmpty() && !isIgnoreTrace && (m_lookAtYVector != 0))
	{
		m_camera.Move(oldEyePos, lookAtPos, camSpeed);
	}
}


void cMapView::OnPreRender(const float deltaSeconds)
{
	double t0 = 0.f, t1 = 0.f, t2 = 0.f;
	t0 = g_global->m_timer.GetSeconds();

	cRenderer &renderer = GetRenderer();
	cAutoCam cam(&m_camera);

	renderer.UnbindTextureAll();

	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	if (m_renderTarget.Begin(renderer, Vector4(0,0,0,1.f)))
	{
		CommonStates states(renderer.GetDevice());

		if (m_showWireframe)
		{
			renderer.GetDevContext()->RSSetState(states.Wireframe());
		}
		else
		{
			renderer.GetDevContext()->RSSetState(states.CullCounterClockwise());
			if (g_global->m_isShowTerrain)
				m_skybox.Render(renderer);
		}

		const Ray ray1 = GetMainCamera().GetRay();
		const Ray ray2 = GetMainCamera().GetRay(m_mousePos.x, m_mousePos.y);

		cFrustum frustum;
		frustum.SetFrustum(GetMainCamera().GetViewProjectionMatrix());
		if (g_global->m_isShowTerrain)
			m_quadTree.Render(renderer, deltaSeconds, frustum, ray1, ray2);

		if (g_global->m_isShowPrevPath)
		{
			for (auto &p : g_global->m_pathRenderers)
				p->Render(renderer);
		}

		//t1 = g_global->m_timer.GetSeconds();

		// render track pos
		auto &track = g_global->m_gpsClient.m_paths;
		if (!track.empty() 
			&& ((g_global->m_camType != eCameraType::Camera1) 
				|| (g_global->m_touch.m_type != eTouchType::Touch)))
		{
			renderer.GetDevContext()->OMSetDepthStencilState(states.DepthNone(), 0);

			Vector3 prevPos;
			const Vector3 camPos = GetMainCamera().GetEyePos();
			renderer.m_dbgLine.SetColor(cColor::RED);
			renderer.m_dbgLine.m_isSolid = true;
			for (int i = 0; i < (int)track.size() - 1; ++i)
			{
				const cGpsClient::sPath &path0 = track[i];
				const cGpsClient::sPath &path1 = track[i+1];
				const Vector3 p0 = m_quadTree.Get3DPos(path0.lonLat);
				const Vector3 p1 = m_quadTree.Get3DPos(path1.lonLat);
				if (p0.Distance(p1) > 100.f)
					continue;

				if (prevPos.IsEmpty())
					prevPos = p0;

				const float len = prevPos.Distance(p1);
				const float camLen = camPos.Distance(p1);
				if (camLen == 0.f)
					continue;

				// ī�޶���� �Ÿ����, ������ ���� ��ġ�� ������� �ʴ´�.
				// ������ 50���� ��ġ�� ����Ѵ�.
				if ((len / camLen < 0.05f) 
					&& (i < (int)(track.size() - 50)))
					continue;

				const float lineWidth = (camLen / 30.f) * 0.06f;
				renderer.m_dbgLine.SetLine(prevPos, p1, lineWidth);
				renderer.m_dbgLine.Render(renderer);
				prevPos = p1;
			}
			renderer.m_dbgLine.m_isSolid = false;

			renderer.GetDevContext()->OMSetDepthStencilState(states.DepthDefault(), 0);

		} // ~if trackpos

		// latLong position
		if ((g_global->m_camType != eCameraType::Camera1)
			|| (g_global->m_touch.m_type != eTouchType::Touch))
		{
			const Vector3 p0 = m_quadTree.Get3DPos(
				{ (double)g_global->m_lonLat.x, (double)g_global->m_lonLat.y });
			renderer.m_dbgLine.m_isSolid = true;
			renderer.m_dbgLine.SetColor(Vector4(1,1,1,0.5f));
			renderer.m_dbgLine.SetLine(p0, p0 + Vector3(0, 0.1f, 0), 0.01f);
			renderer.m_dbgLine.Render(renderer);
			renderer.m_dbgLine.m_isSolid = false;

			//renderer.m_dbgArrow.SetColor(cColor::WHITE);
			//renderer.m_dbgArrow.SetDirection(p0 + Vector3(0, 1, 0)
			//	, p0 + Vector3(0, 1, 0) + m_avrDir * 2.f
			//	, 0.02f);
			//renderer.m_dbgArrow.Render(renderer);


			// ī�޶���� �Ÿ��� ���� ũ�⸦ �����Ѵ�. (�׻� ���� ũ��� ���̱� ����)
			m_curPosObj.m_transform.pos = p0 + Vector3(0, 0.1f, 0);
			const float dist = GetMainCamera().GetEyePos().Distance(p0);
			const float scale = common::clamp(0.02f, 1000.f, (dist * 1.5f) / 180.f);
			m_curPosObj.m_transform.scale = Vector3::Ones * scale;
			m_curPosObj.Render(renderer);
		}

		// render autopilot route
		//if (!g_root.m_route.empty())
		//{
		//	renderer.m_dbgLine.SetColor(cColor::WHITE);
		//	for (int i = 0; i < (int)g_root.m_route.size() - 1; ++i)
		//	{
		//		auto &lonLat0 = g_root.m_route[i];
		//		auto &lonLat1 = g_root.m_route[i + 1];

		//		const Vector3 p0 = gis::GetRelationPos(gis::WGS842Pos(lonLat0));
		//		const Vector3 p1 = gis::GetRelationPos(gis::WGS842Pos(lonLat1));
		//		renderer.m_dbgLine.SetLine(p0 + Vector3(0, 20, 0), p1 + Vector3(0, 20, 0), 0.03f);
		//		renderer.m_dbgLine.Render(renderer);
		//	}
		//}

		if (g_global->m_isShowLandMark)
		{
			for (auto &landMark : g_global->m_landMark.m_landMarks)
			{
				const Vector3 p0 = m_quadTree.Get3DPos(landMark.lonLat);

				// ī�޶���� �Ÿ��� ���� ũ�⸦ �����Ѵ�. (�׻� ���� ũ��� ���̱� ����)
				renderer.m_dbgLine.SetColor(cColor::WHITE);
				renderer.m_dbgLine.SetLine(p0, p0 + Vector3(0, 1, 0), 0.01f);
				renderer.m_dbgLine.Render(renderer);

				m_landMarkObj.m_transform.pos = p0 + Vector3(0, 1, 0);
				const float dist = GetMainCamera().GetEyePos().Distance(p0);
				const float scale = common::clamp(0.5f, 30.f, (dist * 1.5f) / 250.f);
				m_landMarkObj.m_transform.scale = Vector3::Ones * scale;
				m_landMarkObj2.SetColor(cColor::YELLOW);
				m_landMarkObj.Render(renderer);
			}
		}

		if (g_global->m_isShowLandMark2)
		{
			for (auto &landMark : g_global->m_landMark2.m_landMarks)
			{
				const Vector3 p0 = m_quadTree.Get3DPos(landMark.lonLat);

				// ī�޶���� �Ÿ��� ���� ũ�⸦ �����Ѵ�. (�׻� ���� ũ��� ���̱� ����)
				renderer.m_dbgLine.SetColor(cColor::WHITE);
				renderer.m_dbgLine.SetLine(p0, p0 + Vector3(0, 1, 0), 0.01f);
				renderer.m_dbgLine.Render(renderer);

				m_landMarkObj2.m_transform.pos = p0 + Vector3(0, 1, 0);
				const float dist = GetMainCamera().GetEyePos().Distance(p0);
				const float scale = common::clamp(0.5f, 30.f, (dist * 1.5f) / 250.f);
				m_landMarkObj2.m_transform.scale = Vector3::Ones * scale;
				m_landMarkObj2.SetColor(cColor::BLUE);
				m_landMarkObj2.Render(renderer);
			}
		}

		if (g_global->m_isMakeScanPath) 
		{
			renderer.m_dbgLine.SetColor(cColor::WHITE);
			for (int i = 0; i < (int)g_global->m_scanPath.size(); ++i)
			{
				auto &lonLat0 = g_global->m_scanPath[i];
				auto &lonLat1 = g_global->m_scanPath[(i + 1) %
					g_global->m_scanPath.size()];

				const Vector3 p0 = gis::GetRelationPos(gis::WGS842Pos(lonLat0));
				const Vector3 p1 = gis::GetRelationPos(gis::WGS842Pos(lonLat1));
				renderer.m_dbgLine.SetLine(p0 + Vector3(0, 20, 0), p1 + Vector3(0, 20, 0), 0.03f);
				renderer.m_dbgLine.Render(renderer);
			}
		}

		//if (g_global->m_optPath.m_pointMapper)
		//{
		//	renderer.m_dbgBox.SetColor(cColor::WHITE);
		//	cPointMapper *pointMapper = g_global->m_optPath.m_pointMapper;
		//	for (auto kv : pointMapper->m_qtrees)
		//	{
		//		cQuadTree<sMapping>* qtree = kv.second;
		//		sQuadTreeNode<sMapping> *node = qtree->m_roots.empty() ?
		//			nullptr : qtree->m_roots[0];
		//		if (!node)
		//			continue;
		//		for (auto &mpos : node->data.table)
		//		{
		//			const Transform tfm(mpos.pos, Vector3(1, 1, 1)*0.03f);
		//			renderer.m_dbgBox.SetBox(tfm);
		//			renderer.m_dbgBox.Render(renderer);
		//		}
		//	}
		//}

		g_global->m_optPath.RenderQTreeGraph(renderer, m_quadTree);



	}
	m_renderTarget.End(renderer);

	t2 = g_global->m_timer.GetSeconds();

	g_global->m_renderT0 = t2 - t0;
}


void cMapView::OnRender(const float deltaSeconds)
{
	const double t0 = g_global->m_timer.GetSeconds();

	ImVec2 pos = ImGui::GetCursorScreenPos();
	m_viewPos = { (int)(pos.x), (int)(pos.y) };
	m_viewRect = { pos.x + 5, pos.y, pos.x + m_rect.Width() - 30, pos.y + m_rect.Height() - 42 };

	// HUD
	bool isOpen = true;
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;

	// Render Date Information
	const float guageH = m_ledSize + 2.f;
	const float dateH = 43.f - 10.f;
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 1));
	if (g_global->m_isShowMapView)
	{
		ImGui::Image(m_renderTarget.m_resolvedSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 42)
			, ImVec2(0,0), ImVec2(1,1)
			, g_global->m_isDarkMode? *(ImVec4*)&g_global->m_darkColor : ImVec4(1,1,1,1));

		RenderRPMGuage(pos, guageH, deltaSeconds);

		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y));
		ImGui::SetNextWindowSize(ImVec2(m_viewRect.Width(), dateH + guageH));
		if (ImGui::Begin("Car Information", &isOpen, flags))
		{
			// render datetime
			ImGui::PushStyleColor(ImGuiCol_Text, g_global->m_isDarkMode ? ImVec4(1,1,1,0.1f) : ImVec4(1,1,1,1));
			ImGui::PushFont(m_owner->m_fontBig);
			const string dateTime = common::GetCurrentDateTime5();
			ImGui::SetCursorPos(ImVec2(pos.x, 10 + guageH));
			ImGui::Text("%s", dateTime.c_str());

			ImGui::SetCursorPos(ImVec2(m_viewRect.Width() * 0.5f - 8, 0));
			ImGui::Text("%d", g_global->m_gear);

			ImGui::SetCursorPos(ImVec2(m_viewRect.Width() * 0.5f - 15, guageH));
			ImGui::Text("%d km/h", g_global->m_speed);

			ImGui::PopStyleColor();
			ImGui::PopFont();
			ImGui::End();
		}
	}

	// Render Information
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + guageH + dateH));
	ImGui::SetNextWindowBgAlpha(0.f);
	ImGui::SetNextWindowSize(ImVec2(min(m_viewRect.Width(), 400.f), m_viewRect.Height() - 110));
	if (ImGui::Begin("Map Information", &isOpen, flags))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, g_global->m_isDarkMode ? 
			ImVec4(1, 1, 1, 0.1f) : ImVec4(1, 1, 1, 1));
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::PushStyleColor(ImGuiCol_Text, g_global->m_isDarkMode ? 
			ImVec4(1, 1, 0, 0.1f) : ImVec4(1, 1, 0, 1));
		const char *touchStr[] = { "Touch" ,"Gesture", "Mouse" };
		ImGui::Text("%s", touchStr[(int)g_global->m_touch.m_type]);
		ImGui::PopStyleColor();

		const bool isNaviSvrConnect = m_naviClient.IsConnect();
		if (!isNaviSvrConnect)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, g_global->m_isDarkMode ? 
				ImVec4(1, 0, 0, 0.1f) : ImVec4(1, 0, 0, 1));
		}

		ImGui::SameLine();
		ImGui::Text("GPS = %.6f, %.6f, %.1f, (%d)", m_curGpsPos.y, m_curGpsPos.x
			, m_gpsInfo.altitude, m_sendGpsInfo);
		if (!isNaviSvrConnect)
			ImGui::PopStyleColor();
		//ImGui::DragInt("rpm", &g_global->m_rpm);

		if (g_global->m_isShowGPS)
			ImGui::Text(g_global->m_gpsClient.m_recvStr.c_str());

		ImGui::Text("OBD2 = fps:%d, tot:%d, query:%d, %s"
			, m_obd2RcvFps, m_obd2RcvCnt, g_global->m_obd.m_queryCnt
			, g_global->m_obd.m_rcvStr.c_str());

		g_global->m_naviView->OnRender(deltaSeconds);

		ImGui::PopStyleColor();
		ImGui::End();
	}

	// Render Terrain Calculation Time
	if (g_global->m_isShowRenderGraph)
		RenderGraph(pos);

	ImGui::PopStyleColor(); // pop border color

	const double t1 = g_global->m_timer.GetSeconds();
	g_global->m_renderT1 = t1 - t0;
}


// Render RPM GuageBar
void cMapView::RenderRPMGuage(const ImVec2 &pos, const float guageH, const float deltaSeconds)
{
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(ImVec2(m_viewRect.Width(), guageH));
	if (ImGui::Begin("RPM GuageBar", &isOpen, flags))
	{
		// ȭ�鿡 ����� �� �ִ� �ִ� LED ������ ����Ѵ�.
		// blue, green, yellow, red ������ led ��� ������ ����Ѵ�.
		const int maxRPM = m_maxRPM;
		const int blinkRPM = m_blinkRPM;
		const Vector2 ledSize(m_ledSize, m_ledSize);
		const Vector2 offset(5.f, 1.f); // left offset
		const Vector2 roffset(m_viewRect.Width() - ledSize.x - 5.f, 1.f); // right offset
		const float w = m_viewRect.Width() / 2.f;
		const int maxShowLedCount = (int)(w / ledSize.x);
		int maxLed[4];
		maxLed[0] = maxShowLedCount / 4; // blue
		maxLed[1] = maxShowLedCount / 4; // green
		maxLed[2] = maxShowLedCount / 4; // yellow
		maxLed[3] = maxShowLedCount / 4; // red
		int remainCnt = maxShowLedCount % 4;
		maxLed[3] += (remainCnt-- > 0) ? 1 : 0;
		maxLed[2] += (remainCnt-- > 0) ? 1 : 0;
		maxLed[1] += (remainCnt-- > 0) ? 1 : 0;
		const int step = maxRPM / maxShowLedCount;
		maxLed[1] += maxLed[0];
		maxLed[2] += maxLed[1];
		maxLed[3] += maxLed[2];

		// RPM�� blinkRPM ���� ũ�ٸ� led�� �����Ų��.
		const float timeLEDBlink = 0.1f;
		if (g_global->m_rpm > blinkRPM)
		{
			m_ledBlinkTime += deltaSeconds;
			if (m_ledBlinkTime > timeLEDBlink * 2.f)
				m_ledBlinkTime = 0.f;
		}
		else
		{
			m_ledBlinkTime = 0.f;
		}

		if (m_ledBlinkTime < timeLEDBlink)
		{
			const int lv = g_global->m_rpm / step;
			const int tailLv = g_global->m_rpm % step;
			const float tailR = (float)tailLv / (float)step;
			int ledColor = 4;
			for (int i = 0; i < 4; ++i)
			{
				if (maxLed[i] > lv)
				{
					ledColor = i;
					break;
				}
			}

			for (int i = 0; i < lv; ++i)
			{
				for (int k = 0; k < 4; ++k)
				{
					if (maxLed[k] > i)
					{
						// last guagebar step, size scaling
						Vector2 center(0, 0); // center offset
						Vector2 size = ledSize;
						if (i + 1 == lv)
						{
							const float a = m_ledAniTailR;
							const float r = min(1.f, tailR * a + (1.f - a));
							size = ledSize * r;
							center = ledSize * (1 - r) * 0.5f;
						}
						else
						{
							const float a = m_ledAniR;
							const float r = min(1.f, tailR * 0.1f + (1.f - a));
							size = ledSize * r;
							center = ledSize * (1 - r) * 0.5f;
						}

						// render left
						const Vector2 lp = offset + center + Vector2(ledSize.x * i, 0);
						ImGui::SetCursorPos(*(ImVec2*)&lp);
						ImGui::Image(m_ledTexture[k]->m_texSRV, *(ImVec2*)&size);

						// render right
						const Vector2 rp = roffset + center - Vector2(ledSize.x * i, 0);
						ImGui::SetCursorPos(*(ImVec2*)&rp);
						ImGui::Image(m_ledTexture[k]->m_texSRV, *(ImVec2*)&size);
						break;
					}
				}
			}
		}

		ImGui::End();
	}
}


void cMapView::RenderGraph(const ImVec2 &pos)
{
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;

	if (g_global->m_isCalcRenderGraph)
	{
		switch (g_global->m_analysisType)
		{
		case eAnalysisType::MapView:
			m_renderOverhead[0][m_graphIdx] = (float)(g_global->m_renderT0 + g_global->m_renderT1);
			m_renderOverhead[1][m_graphIdx] = (float)g_global->m_renderT0;
			m_renderOverhead[2][m_graphIdx] = (float)g_global->m_renderT1;
			m_renderOverhead[3][m_graphIdx] = (float)m_quadTree.m_t2;
			m_renderOverhead[4][m_graphIdx] = (float)m_quadTree.m_tileMgr->m_cntRemoveTile;
			break;

		case eAnalysisType::Terrain:
			m_renderOverhead[0][m_graphIdx] = (float)m_quadTree.m_t0;
			m_renderOverhead[1][m_graphIdx] = (float)m_quadTree.m_t1;
			m_renderOverhead[2][m_graphIdx] = (float)m_quadTree.m_t2;
			m_renderOverhead[3][m_graphIdx] = (float)m_quadTree.m_tileMgr->m_cntRemoveTile;
			break;

		case eAnalysisType::GMain:
			m_renderOverhead[0][m_graphIdx] = (float)g_application->m_deltaSeconds;
			//m_renderOverhead[1][m_graphIdx] = (float)m_owner->m_t0;
			//m_renderOverhead[2][m_graphIdx] = (float)m_owner->m_t1;
			//m_renderOverhead[3][m_graphIdx] = (float)m_owner->m_t2;
			//m_renderOverhead[4][m_graphIdx] = (float)m_owner->m_t3;
			break;

		default: assert(0); break;
		}

		m_graphIdx++;
		m_graphIdx %= 500;
	}

	ImGui::SetNextWindowBgAlpha(0.f);
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + 170.f));
	ImGui::SetNextWindowSize(ImVec2(min(m_viewRect.Width(), 700.f)
		, min(m_viewRect.Height() - 100.f, 550.f)));
	if (ImGui::Begin("Render Graph", &isOpen, flags))
	{
		const ImVec4 bgColor = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(bgColor.x, bgColor.y, bgColor.z, 0.5f));

		switch (g_global->m_analysisType)
		{
		case eAnalysisType::MapView:
		{
			ImGui::PlotLines("graph0", m_renderOverhead[0]
				, IM_ARRAYSIZE(m_renderOverhead[0])
				, m_graphIdx, "pre + onrender", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph1", m_renderOverhead[1]
				, IM_ARRAYSIZE(m_renderOverhead[1])
				, m_graphIdx, "pre render ", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph2", m_renderOverhead[2]
				, IM_ARRAYSIZE(m_renderOverhead[2])
				, m_graphIdx, "on render ", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph3", m_renderOverhead[3]
				, IM_ARRAYSIZE(m_renderOverhead[3])
				, m_graphIdx, "update", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph4", m_renderOverhead[4]
				, IM_ARRAYSIZE(m_renderOverhead[4])
				, m_graphIdx, "remove", 0, 10.f, ImVec2(700, 100));
		}
		break;

		case eAnalysisType::Terrain:
		{
			ImGui::PlotLines("graph0", m_renderOverhead[0]
				, IM_ARRAYSIZE(m_renderOverhead[0])
				, m_graphIdx, "build", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph1", m_renderOverhead[1]
				, IM_ARRAYSIZE(m_renderOverhead[1])
				, m_graphIdx, "render", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph3", m_renderOverhead[2]
				, IM_ARRAYSIZE(m_renderOverhead[2])
				, m_graphIdx, "update", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph4", m_renderOverhead[3]
				, IM_ARRAYSIZE(m_renderOverhead[3])
				, m_graphIdx, "remove", 0, 10.f, ImVec2(700, 100));
		}
		break;

		case eAnalysisType::GMain:
		{
			ImGui::PlotLines("graph0", m_renderOverhead[0]
				, IM_ARRAYSIZE(m_renderOverhead[0])
				, m_graphIdx, "dt", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph1", m_renderOverhead[1]
				, IM_ARRAYSIZE(m_renderOverhead[1])
				, m_graphIdx, "render window event", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph2", m_renderOverhead[2]
				, IM_ARRAYSIZE(m_renderOverhead[2])
				, m_graphIdx, "render window update", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph3", m_renderOverhead[3]
				, IM_ARRAYSIZE(m_renderOverhead[3])
				, m_graphIdx, "render window pre render", 0, 0.05f, ImVec2(700, 100));

			ImGui::PlotLines("graph4", m_renderOverhead[4]
				, IM_ARRAYSIZE(m_renderOverhead[4])
				, m_graphIdx, "render window on render", 0, 0.05f, ImVec2(700, 100));
		}
		break;

		default: assert(0); break;
		}


		ImGui::PopStyleColor();
	}
	ImGui::End();
}


// change trace camera variable
void cMapView::ChangeViewCamera(const eCameraType camType)
{
	if (eCameraType::Custom != camType)
	{
		m_lookAtDistance = g_global->m_camInfo[(int)camType].distance;
		m_lookAtYVector = g_global->m_camInfo[(int)camType].lookAtY;
	}
}


void cMapView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect) 
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


void cMapView::UpdateLookAt()
{
	GetMainCamera().MoveCancel();

	const float centerX = GetMainCamera().m_width / 2;
	const float centerY = GetMainCamera().m_height / 2;
	const Ray ray = GetMainCamera().GetRay((int)centerX, (int)centerY);
	const float distance = m_groundPlane1.Collision(ray.dir);
	if (distance < -0.2f)
	{
		GetMainCamera().m_lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
	}
	else
	{ // horizontal viewing
		const Vector3 lookAt = GetMainCamera().m_eyePos + GetMainCamera().GetDirection() * 50.f;
		GetMainCamera().m_lookAt = lookAt;
	}

	GetMainCamera().UpdateViewMatrix();
}


// ���� �������� ��,
// ī�޶� �տ� �ڽ��� �ִٸ�, �ڽ� ���鿡�� �����.
void cMapView::OnWheelMove(const float delta, const POINT mousePt)
{
	UpdateLookAt();

	float len = 0;
	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	Vector3 lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
	len = (ray.orig - lookAt).Length();

	const int lv = m_quadTree.GetLevel(len);
	float zoomLen = min(len * 0.1f, (float)(2 << (16-lv)));

	if (m_isGestureInput && g_global->m_touch.IsGestureMode())
	{
		zoomLen *= 5.f;
	}

	GetMainCamera().Zoom(ray.dir, (delta < 0) ? -zoomLen : zoomLen);

	// �������� �Ʒ��� ī�޶� ��ġ�ϸ�, ���̸� �����Ѵ�.
	const Vector3 eyePos = m_camera.GetEyePos();
	const float h = m_quadTree.GetHeight(Vector2(eyePos.x, eyePos.z));
	const float offset = 0.01f;
	if (eyePos.y < (h + offset))
	{
		const Vector3 newPos(eyePos.x, h + offset, eyePos.z);
		m_camera.SetEyePos(newPos);
	}

	UpdateCameraTraceLookat();
}


// �̵� ��ο��� �Ÿ��� ������Ʈ �Ѵ�.
// �� �Ÿ��� �����ϸ鼭 GPS ��ǥ�� �����Ѵ�.
void cMapView::UpdateCameraTraceLookat(
	const bool isUpdateDistance //=true
)
{
	auto &track = g_global->m_gpsClient.m_paths;
	if (g_global->m_isTraceGPSPos 
		&& !track.empty()
		&& (g_global->m_camType == eCameraType::Custom))
	{
		const Vector3 p0 = m_quadTree.Get3DPos(track.back().lonLat);
		if (isUpdateDistance)
			m_lookAtDistance = min(200.f, m_camera.GetEyePos().Distance(p0));
		//m_lookAtYVector = m_camera.GetDirection().y;
	}
}


// Gesture input
void cMapView::OnGestured(const int id, const POINT mousePt)
{
	static bool isGesturePanInput = false;

	switch (id)
	{
	case GID_BEGIN:
		m_isGestureInput = true;
		break;

	case GID_END:
		isGesturePanInput = false;
		m_isGestureInput = true;
		m_mouseDown[1] = false;
		break;

	case GID_PAN:
		if (isGesturePanInput)
		{
			m_mouseDown[1] = true;
			OnMouseMove(mousePt);
		}
		else
		{
			isGesturePanInput = true;
			m_mousePos = mousePt;
			UpdateLookAt();
		}
		break;

	case GID_TWOFINGERTAP:
	case GID_PRESSANDTAP:
		if (g_global->m_touch.IsGestureMode(m_owner->getSystemHandle()))
			g_global->m_touch.SetTouchMode(m_owner->getSystemHandle());
		else
			g_global->m_touch.SetGestureMode(m_owner->getSystemHandle());
		break;
	}
}


// Handling Mouse Move Event
void cMapView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;

	if (m_mouseDown[0])
	{
		Vector3 dir = GetMainCamera().GetDirection();
		Vector3 right = GetMainCamera().GetRight();
		dir.y = 0;
		dir.Normalize();
		right.y = 0;
		right.Normalize();

		GetMainCamera().MoveRight(-delta.x * m_rotateLen * 0.001f);
		GetMainCamera().MoveFrontHorizontal(delta.y * m_rotateLen * 0.001f);
		UpdateCameraTraceLookat();
	}
	else if (m_mouseDown[1])
	{
		const float scale = (m_isGestureInput)? 0.001f : 0.005f;
		m_camera.Yaw2(delta.x * scale, Vector3(0, 1, 0));
		m_camera.Pitch2(delta.y * scale, Vector3(0, 1, 0));
		UpdateCameraTraceLookat();
	}
	else if (m_mouseDown[2])
	{
		const float len = GetMainCamera().GetDistance();
		GetMainCamera().MoveRight(-delta.x * len * 0.001f);
		GetMainCamera().MoveUp(delta.y * len * 0.001f);
		UpdateCameraTraceLookat();
	}

	// �������� �Ʒ��� ī�޶� ��ġ�ϸ�, ���̸� �����Ѵ�.
	const Vector3 eyePos = m_camera.GetEyePos();
	const float h = m_quadTree.GetHeight(Vector2(eyePos.x, eyePos.z));
	const float offset = 0.01f;
	if (eyePos.y < (h + offset))
	{
		const Vector3 newPos(eyePos.x, h + offset, eyePos.z);
		m_camera.SetEyePos(newPos);
	}
}


// Handling Mouse Button Down Event
void cMapView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
{
	m_mousePos = mousePt;
	UpdateLookAt();
	SetCapture();

	switch (button)
	{
	case sf::Mouse::Left:
	{
		m_mouseDown[0] = true;
		const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
		auto result = m_quadTree.Pick(ray);
		Vector3 p1 = result.second;
		m_rotateLen = (p1 - ray.orig).Length();

		cAutoCam cam(&m_camera);
		const Vector2d lonLat = m_quadTree.GetWGS84(p1);
		gis::LatLonToUTMXY(lonLat.y, lonLat.x, 52, g_global->m_utmLoc.x, g_global->m_utmLoc.y);
		g_global->m_lonLat = Vector2((float)lonLat.x, (float)lonLat.y);

		if (g_global->m_isSelectMapScanningCenter)
		{
			g_global->m_scanCenter = lonLat;
			g_global->m_isSelectMapScanningCenter = false;
		}
		else if (g_global->m_isMakeScanPath && GetAsyncKeyState(VK_LCONTROL))
		{
			cGpsClient::sPath path;
			path.t = 0;
			path.lonLat = lonLat;
			//g_global->m_gpsClient.m_paths.push_back(path);
			g_global->m_scanPath.push_back(lonLat);
		}
		else if (g_global->m_landMarkSelectState == 1)
		{
			g_global->m_landMarkSelectState = 0;
			g_global->m_landMark.AddLandMark("LandMark", lonLat);
			if (m_naviClient.IsConnect())
				m_gpsProtocol.AddLandMark(network2::SERVER_NETID, lonLat.x, lonLat.y);
		}
		else if (g_global->m_landMarkSelectState2 == 1)
		{
			g_global->m_landMarkSelectState2 = 0;
			g_global->m_landMark2.AddLandMark("LandMark", lonLat);
		}
	}
	break;

	case sf::Mouse::Right:
	{
		m_mouseDown[1] = true;

		const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
		Vector3 target = m_groundPlane1.Pick(ray.orig, ray.dir);
		const float len = (GetMainCamera().GetEyePos() - target).Length();
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = true;
		break;
	}
}


void cMapView::OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	ReleaseCapture();

	switch (button)
	{
	case sf::Mouse::Left:
		m_mouseDown[0] = false;
		break;
	case sf::Mouse::Right:
		m_mouseDown[1] = false;
		break;
	case sf::Mouse::Middle:
		m_mouseDown[2] = false;
		break;
	}
}


void cMapView::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		switch (evt.key.cmd)
		{
		case sf::Keyboard::Return:
			break;

		case sf::Keyboard::Space:
		{
			// 128.467758, 37.171993
			//m_camera.Move();
		}
		break;

		case sf::Keyboard::Left: 
			m_camera.MoveRight2(-0.5f); 
			UpdateCameraTraceLookat(false);
			break;

		case sf::Keyboard::Right: 
			m_camera.MoveRight2(0.5f); 
			UpdateCameraTraceLookat(false);
			break;

		case sf::Keyboard::Up: 
			m_camera.MoveUp2(0.5f);
			UpdateCameraTraceLookat(false);
			break;

		case sf::Keyboard::Down: 
			m_camera.MoveUp2(-0.5f);
			UpdateCameraTraceLookat(false);
			break;
		}
		break;

	case sf::Event::MouseMoved:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnMouseMove(pos);
	}
	break;

	case sf::Event::MouseButtonPressed:
	case sf::Event::MouseButtonReleased:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		if (sf::Event::MouseButtonPressed == evt.type)
		{
			if (m_viewRect.IsIn((float)curPos.x, (float)curPos.y))
				OnMouseDown(evt.mouseButton.button, pos);
		}
		else
		{
			OnMouseUp(evt.mouseButton.button, pos);
		}
	}
	break;

	case sf::Event::MouseWheelScrolled:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnWheelMove(evt.mouseWheelScroll.delta, pos);
	}
	break;

	case sf::Event::Gestured:
	{
		POINT curPos = { evt.gesture.x, evt.gesture.y };
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnGestured(evt.gesture.id, pos);
	}
	break;

	}
}


void cMapView::OnResetDevice()
{
	cRenderer &renderer = GetRenderer();

	// update viewport
	sRectf viewRect = { 0, 0, m_rect.Width() - 15, m_rect.Height() - 50 };
	m_camera.SetViewPort(viewRect.Width(), viewRect.Height());

	cViewport vp = GetRenderer().m_viewPort;
	vp.m_vp.Width = viewRect.Width();
	vp.m_vp.Height = viewRect.Height();
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, DXGI_FORMAT_D24_UNORM_S8_UINT);
}

