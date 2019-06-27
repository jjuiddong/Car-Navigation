
#include "stdafx.h"
#include "mapview.h"

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
{
	ZeroMemory(m_renderOverhead, sizeof(m_renderOverhead));
}

cMapView::~cMapView() 
{
	m_quadTree.Clear();
}


bool cMapView::Init(cRenderer &renderer) 
{
	const Vector3 eyePos(331843.563f, 478027.719f, 55058.8672f);
	const Vector3 lookAt(331004.031f, 0.000000000f, 157745.281f);
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, m_rect.Width() / m_rect.Height(), 1.f, 1000000.f);
	m_camera.SetViewPort(m_rect.Width(), m_rect.Height());

	sf::Vector2u size((u_int)m_rect.Width() - 15, (u_int)m_rect.Height() - 50);
	cViewport vp = renderer.m_viewPort;
	vp.m_vp.Width = (float)size.x;
	vp.m_vp.Height = (float)size.y;
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true
		, DXGI_FORMAT_D24_UNORM_S8_UINT);

	if (!m_quadTree.Create(renderer, true))
		return false;

	m_quadTree.m_isShowDistribute = false;
	m_quadTree.m_isShowQuadTree = false;
	m_quadTree.m_isShowFacility = false;
	m_quadTree.m_isShowPoi1 = true;
	m_quadTree.m_isShowPoi2 = false;

	m_skybox.Create(renderer, "../media/skybox/sky.dds");
	m_skybox.m_isDepthNone = true;

	m_curPosObj.Create(renderer, 1.f, 10, 10
		, (eVertexType::POSITION | eVertexType::NORMAL), cColor::RED);

	return true;
}


void cMapView::OnUpdate(const float deltaSeconds)
{
	// 카메라가 가르키는 방향의 경위도를 구한다.
	const Vector2d camLonLat = m_quadTree.GetLongLat(m_camera.GetRay());

	if (g_global->m_isShowTerrain)
		m_quadTree.Update(GetRenderer(), camLonLat, deltaSeconds);
	m_camera.Update(deltaSeconds);

	g_global->m_touch.CheckTouchType(m_owner->getSystemHandle());

	UpdateGPS();
	UpdateMapScanning(deltaSeconds);
}


// GPS 서버로 부터 위치정보를 받아 업데이트 한다.
// receive from gps server (mobile phone)
void cMapView::UpdateGPS()
{
	const float MIN_LENGTH = 0.5f;

	gis::sGPRMC gpsInfo;
	if (!g_global->m_gpsClient.GetGpsInfo(gpsInfo))
		return;

	m_gpsInfo = gpsInfo;
	m_curGpsPos = gpsInfo.lonLat;

	static Vector2d oldGpsPos;
	static Vector3 oldEyePos;
	if (oldGpsPos.IsEmpty())
		oldGpsPos = m_curGpsPos;

	if (m_lookAtDistance == 0)
	{
		m_lookAtDistance = m_camera.GetEyePos().Distance(m_camera.GetLookAt());
		m_lookAtYVector = m_camera.GetDirection().y;
	}

	// path 로그 파일에 저장
	if (g_global->m_gpsClient.IsConnect())
	{
		const string date = common::GetCurrentDateTime();
		dbg::Logp2("path.txt", "%s, %.15f, %.15f\n"
			, date.c_str(), m_curGpsPos.x, m_curGpsPos.y);
	}

	// 현재 위치를 향해 카메라 looAt을 조정한다.
	// lookAt이 바뀜에 따라 카메라 위치도 기존 거리를 유지하며 이동한다.
	g_root.m_lonLat = Vector2((float)m_curGpsPos.x, (float)m_curGpsPos.y);
	const Vector3 pos = m_quadTree.Get3DPos(m_curGpsPos);
	const Vector3 oldPos = m_quadTree.Get3DPos(oldGpsPos);

	const bool isTraceGPSPos = g_global->m_isTraceGPSPos
		&& !m_mouseDown[0] && !m_mouseDown[1] 
		&& (g_global->m_touch.m_type != eTouchType::Gesture);

	// 카메라 방향을 바꾼다. 이동하는 방향으로 향하게 한다.
	// 최근 이동 궤적에서 n개의 벡터를 평균해서 최종 방향을 결정한다. (가중치 평균)
	Vector3 avrDir;
	{
		// 최근 n1개 50% 가중치
		// 나머지 n2개 50% 가중치
		const int n1 = 10;
		const int n2 = 50;
		const float r1 = 50.f / (float)n1;
		const float r2 = 50.f / (float)n2;

		int cnt = 0;
		auto &track = g_global->m_gpsClient.m_paths;
		for (int i = (int)track.size() - 1; (i >= 1) && (cnt < (n1 + n2)); --i, ++cnt)
		{
			const Vector2d &ll0 = track[i - 1].lonLat;
			const Vector2d &ll1 = track[i].lonLat;
			const Vector3 p0 = m_quadTree.Get3DPos(ll0);
			const Vector3 p1 = m_quadTree.Get3DPos(ll1);
			Vector3 d = p1 - p0;
			d.y = 0.f;
			d.Normalize();
			if (cnt < n1)
				avrDir += d * r1;
			else
				avrDir += d * r2;
		}
		avrDir.Normalize();
		m_avrDir = avrDir;
	}

	if (oldPos.Distance(pos) > MIN_LENGTH)
	{
		Vector3 dir = avrDir;
		dir.y = m_lookAtYVector;
		const Vector3 newEyePos = pos + dir * -m_lookAtDistance;

		// 제스처 입력 시에는 카메라를 자동으로 움직이지 않는다.
		if (isTraceGPSPos)
			m_camera.Move(newEyePos, pos, 30.0f);

		oldGpsPos = m_curGpsPos;
		oldEyePos = newEyePos;
	}
	else if (isTraceGPSPos)
	{
		m_camera.Move(oldEyePos, pos, 30.0f);
	}

	// 이동궤적 저장
	// 그전 위치와 거의 같다면 저장하지 않는다.
	{
		auto &track = g_global->m_gpsClient.m_paths;
		const Vector3 lastPos = track.empty() ? Vector3::Zeroes : m_quadTree.Get3DPos(track.back().lonLat);

		if (track.empty()
			|| (!track.empty() && (lastPos.Distance(pos) > MIN_LENGTH)))
			track.push_back({ common::GetCurrentDateTime3(), gpsInfo.lonLat });
	}
}


// 카메라를 이동하면서 파일을 다운로드 받는다.
// 카메라 위치로 위경도를 파악하고, 특정 범위안에서 움직이도록 한다.
void cMapView::UpdateMapScanning(const float deltaSeconds)
{
	if (!g_global->m_isMapScanning)
		return;

	// 파일을 다운로드 중이라면 대기한다.
	if (m_quadTree.m_tileMgr.m_vworldDownloader.m_requestIds.size() > 0)
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
	g_global->m_scanDir += dir * 0.3f * deltaSeconds; // 조금씩 밖으로 방향을 튼다.
	g_global->m_scanPos = nextPos;
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

	if (m_renderTarget.Begin(renderer))
	{
		CommonStates states(renderer.GetDevice());

		if (m_showWireframe)
		{
			renderer.GetDevContext()->RSSetState(states.Wireframe());
		}
		else
		{
			renderer.GetDevContext()->RSSetState(states.CullCounterClockwise());
			m_skybox.Render(renderer);
		}

		const Ray ray1 = GetMainCamera().GetRay();
		const Ray ray2 = GetMainCamera().GetRay(m_mousePos.x, m_mousePos.y);

		cFrustum frustum;
		frustum.SetFrustum(GetMainCamera().GetViewProjectionMatrix());
		if (g_global->m_isShowTerrain)
			m_quadTree.Render(renderer, deltaSeconds, frustum, ray1, ray2);

		//t1 = g_global->m_timer.GetSeconds();

		// latLong position
		{
			const Vector3 p0 = m_quadTree.Get3DPos({ (double)g_root.m_lonLat.x, (double)g_root.m_lonLat.y });
			renderer.m_dbgLine.SetColor(cColor::WHITE);
			renderer.m_dbgLine.SetLine(p0, p0 + Vector3(0, 1, 0), 0.01f);
			renderer.m_dbgLine.Render(renderer);

			renderer.m_dbgArrow.SetColor(cColor::WHITE);
			renderer.m_dbgArrow.SetDirection(p0 + Vector3(0, 1, 0)
				, p0 + Vector3(0, 1, 0) + m_avrDir * 2.f
				, 0.02f);
			renderer.m_dbgArrow.Render(renderer);


			// 카메라와의 거리에 따라 크기를 변경한다. (항상 같은 크기로 보이기 위함)
			m_curPosObj.m_transform.pos = p0 + Vector3(0, 1, 0);
			const float dist = GetMainCamera().GetEyePos().Distance(p0);
			const float scale = common::clamp(0.2f, 1000.f, (dist * 1.5f) / 140.f);
			m_curPosObj.m_transform.scale = Vector3::Ones * scale;
			m_curPosObj.Render(renderer);
		}

		// render autopilot route
		if (!g_root.m_route.empty())
		{
			renderer.m_dbgLine.SetColor(cColor::WHITE);
			for (int i=0; i < (int)g_root.m_route.size()-1; ++i)
			{
				auto &lonLat0 = g_root.m_route[i];
				auto &lonLat1 = g_root.m_route[i+1];

				const Vector3 p0 = gis::GetRelationPos(gis::WGS842Pos(lonLat0));
				const Vector3 p1 = gis::GetRelationPos(gis::WGS842Pos(lonLat1));
				renderer.m_dbgLine.SetLine(p0+ Vector3(0, 20, 0), p1 + Vector3(0, 20, 0), 0.03f);
				renderer.m_dbgLine.Render(renderer);
			}
		}

		// render track pos
		auto &track = g_global->m_gpsClient.m_paths;
		if (!track.empty())
		{
			renderer.GetDevContext()->OMSetDepthStencilState(states.DepthNone(), 0);

			Vector3 prevPos;
			const Vector3 camPos = GetMainCamera().GetEyePos();
			renderer.m_dbgLine.SetColor(cColor::RED);
			for (int i = 0; i < (int)track.size() - 1; ++i)
			{
				const Vector2d &ll0 = track[i].lonLat;
				const Vector2d &ll1 = track[i + 1].lonLat;
				const Vector3 p0 = m_quadTree.Get3DPos(ll0);
				const Vector3 p1 = m_quadTree.Get3DPos(ll1);
				if (p0.Distance(p1) > 100.f)
					continue;

				if (prevPos.IsEmpty())
					prevPos = p0;

				const float len = prevPos.Distance(p1);
				const float camLen = camPos.Distance(p1);
				if (camLen == 0.f)
					continue;

				// 카메라와의 거리대비, 간격이 좁은 위치는 출력하지 않는다.
				// 마지막 20개의 위치는 출력한다.
				if ((len / camLen < 0.05f) 
					&& (i < (int)(track.size() - 20)))
					continue;

				renderer.m_dbgLine.SetLine(prevPos, p1, 0.03f);
				renderer.m_dbgLine.Render(renderer);
				prevPos = p1;
			}

			renderer.GetDevContext()->OMSetDepthStencilState(states.DepthDefault(), 0);

		} // ~if trackpos
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
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

	// Render Date Information
	const int dateH = 43;
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

	if (g_global->m_isShowMapView)
	{
		ImGui::Image(m_renderTarget.m_resolvedSRV, ImVec2(m_rect.Width() - 15, m_rect.Height() - 42));

		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowBgAlpha(0.4f);
		ImGui::SetNextWindowSize(ImVec2(min(m_viewRect.Width(), 400), dateH));
		if (ImGui::Begin("Date Information", &isOpen, flags))
		{
			// render datetime
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
			ImGui::PushFont(m_owner->m_fontBig);
			const string dateTime = common::GetCurrentDateTime5();
			ImGui::Text("%s, Speed: %.1f", dateTime.c_str(), m_gpsInfo.speed);
			ImGui::PopStyleColor();
			ImGui::PopFont();
			ImGui::End();
		}
	}

	// Render Information
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + dateH));
	ImGui::SetNextWindowBgAlpha(0.f);
	ImGui::SetNextWindowSize(ImVec2(min(m_viewRect.Width(), 400), 300));
	if (ImGui::Begin("Map Information", &isOpen, flags))
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));

		const char *touchStr[] = { "Touch" ,"Gesture", "Mouse" };
		ImGui::Text("%s", touchStr[(int)g_global->m_touch.m_type]);
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::Text("GPS = %.6f, %.6f", m_curGpsPos.y, m_curGpsPos.x);

		if (g_global->m_isShowGPS)
			ImGui::Text(g_global->m_gpsClient.m_recvStr.c_str());
		ImGui::End();
	}

	// Render Terrain Calculation Time
	if (g_global->m_isShowRenderGraph)
	{
		if (g_global->m_isCalcRenderGraph)
		{
			switch (g_global->m_analysisType)
			{
			case eAnalysisType::MapView:
				m_renderOverhead[0][m_graphIdx] = (float)(g_global->m_renderT0 + g_global->m_renderT1);
				m_renderOverhead[1][m_graphIdx] = (float)g_global->m_renderT0;
				m_renderOverhead[2][m_graphIdx] = (float)g_global->m_renderT1;
				m_renderOverhead[3][m_graphIdx] = (float)m_quadTree.m_t2;
				m_renderOverhead[4][m_graphIdx] = (float)m_quadTree.m_tileMgr.m_cntRemoveTile;
				break;

			case eAnalysisType::Terrain:
				m_renderOverhead[0][m_graphIdx] = (float)m_quadTree.m_t0;
				m_renderOverhead[1][m_graphIdx] = (float)m_quadTree.m_t1;
				m_renderOverhead[2][m_graphIdx] = (float)m_quadTree.m_t2;
				m_renderOverhead[3][m_graphIdx] = (float)m_quadTree.m_tileMgr.m_cntRemoveTile;
				break;

			case eAnalysisType::GMain:
				m_renderOverhead[0][m_graphIdx] = (float)g_application->m_deltaSeconds;
				m_renderOverhead[1][m_graphIdx] = (float)m_owner->m_t0;
				m_renderOverhead[2][m_graphIdx] = (float)m_owner->m_t1;
				m_renderOverhead[3][m_graphIdx] = (float)m_owner->m_t2;
				m_renderOverhead[4][m_graphIdx] = (float)m_owner->m_t3;
				break;

			default: assert(0); break;
			}

			m_graphIdx++;
			m_graphIdx %= 500;
		}

		ImGui::SetNextWindowBgAlpha(0.f);
		ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + 300));
		ImGui::SetNextWindowSize(ImVec2(min(m_viewRect.Width(), 700)
			, min(m_viewRect.Height() - 100, 550)));
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
	ImGui::PopStyleColor();

	const double t1 = g_global->m_timer.GetSeconds();
	g_global->m_renderT1 = t1 - t0;
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


// 휠을 움직였을 때,
// 카메라 앞에 박스가 있다면, 박스 정면에서 멈춘다.
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

	// 지형보다 아래에 카메라가 위치하면, 높이를 조절한다.
	const Vector3 eyePos = m_camera.GetEyePos();
	const float h = m_quadTree.GetHeight(Vector2(eyePos.x, eyePos.z));
	if (eyePos.y < (h + 2))
	{
		const Vector3 newPos(eyePos.x, h + 2, eyePos.z);
		m_camera.SetEyePos(newPos);
	}

	// 이동 경로와의 거리를 업데이트 한다.
	// 이 거리를 유지하면서 GPS 좌표를 추적한다.
	auto &track = g_global->m_gpsClient.m_paths;
	if (g_global->m_isTraceGPSPos && !track.empty())
	{
		const Vector3 p0 = m_quadTree.Get3DPos(track.back().lonLat);
		m_lookAtDistance = m_camera.GetEyePos().Distance(p0);
		m_lookAtYVector = m_camera.GetDirection().y;
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
	}
	else if (m_mouseDown[1])
	{
		const float scale = (m_isGestureInput)? 0.001f : 0.005f;
		m_camera.Yaw2(delta.x * scale, Vector3(0, 1, 0));
		m_camera.Pitch2(delta.y * scale, Vector3(0, 1, 0));
	}
	else if (m_mouseDown[2])
	{
		const float len = GetMainCamera().GetDistance();
		GetMainCamera().MoveRight(-delta.x * len * 0.001f);
		GetMainCamera().MoveUp(delta.y * len * 0.001f);
	}

	// 지형보다 아래에 카메라가 위치하면, 높이를 조절한다.
	const Vector3 eyePos = m_camera.GetEyePos();
	const float h = m_quadTree.GetHeight(Vector2(eyePos.x, eyePos.z));
	if (eyePos.y < (h + 2))
	{
		const Vector3 newPos(eyePos.x, h + 2, eyePos.z);
		m_camera.SetEyePos(newPos);
	}

	// 이동 경로와의 거리를 업데이트 한다.
	// 이 거리를 유지하면서 GPS 좌표를 추적한다.
	auto &track = g_global->m_gpsClient.m_paths;
	if (g_global->m_isTraceGPSPos && !track.empty())
	{
		const Vector3 p0 = m_quadTree.Get3DPos(track.back().lonLat);
		m_lookAtDistance = m_camera.GetEyePos().Distance(p0);
		m_lookAtYVector = m_camera.GetDirection().y;
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
		gis::LatLonToUTMXY(lonLat.y, lonLat.x, 52, g_root.m_utmLoc.x, g_root.m_utmLoc.y);
		g_root.m_lonLat = Vector2((float)lonLat.x, (float)lonLat.y);

		if (g_global->m_isSelectMapScanningCenter)
		{
			g_global->m_scanCenter = lonLat;
			g_global->m_isSelectMapScanningCenter = false;
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
		switch (evt.key.code)
		{
		case sf::Keyboard::Return:
			break;

		case sf::Keyboard::Space:
		{
			// 128.467758, 37.171993
			//m_camera.Move();
		}
		break;

		case sf::Keyboard::Left: m_camera.MoveRight2(-0.5f); break;
		case sf::Keyboard::Right: m_camera.MoveRight2(0.5f); break;
		case sf::Keyboard::Up: m_camera.MoveUp2(0.5f); break;
		case sf::Keyboard::Down: m_camera.MoveUp2(-0.5f); break;
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

