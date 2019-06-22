
#include "stdafx.h"
#include "navigationview.h"
#include "carnavi.h"
#include "mapview.h"

using namespace graphic;
using namespace framework;


cNavigationView::cNavigationView(const StrId &name)
	: framework::cDockWindow(name)
{

	// 시리얼 포트 열거, 콤보 박스 문자열로 변환한다.
	common::EnumCOMPort(m_ports);
	string str;
	for (auto &port : m_ports)
	{
		StrId id = port.second;
		str += id.utf8().c_str();
		str += "^";
	}
	if (str.length() < m_comboStr.SIZE)
	{
		memcpy(m_comboStr.m_str, str.c_str(), str.length());
		for (auto &c : m_comboStr.m_str)
			if (c == '^')
				c = '\0';
	}

}

cNavigationView::~cNavigationView()
{
}


void cNavigationView::OnUpdate(const float deltaSeconds)
{

}


void cNavigationView::OnRender(const float deltaSeconds)
{
	cViewer *viewer = (cViewer*)g_application;
	cTerrainQuadTree &terrain = viewer->m_mapView->m_quadTree;
	cGpsClient &gpsClient = g_global->m_gpsClient;

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
	ImGui::Text(g_global->m_touch.IsTouchMode() ? "Touch Mode" : "Gesture Mode");
	ImGui::PopStyleColor();

	ImGui::RadioButton("Serial", (int*)&gpsClient.m_inputType
		, (int)cGpsClient::eInputType::Serial);
	ImGui::SameLine();
	ImGui::RadioButton("Network", (int*)&gpsClient.m_inputType
		, (int)cGpsClient::eInputType::Network);

	if (cGpsClient::eInputType::Serial == gpsClient.m_inputType)
	{
		static int com = 0;
		ImGui::Text("Serial");
		ImGui::PushItemWidth(70);
		ImGui::Combo("Port", &com, m_comboStr.c_str());
		ImGui::SameLine();
		ImGui::PopItemWidth();

		if (gpsClient.m_serial.IsOpen())
		{
			if (ImGui::Button("Close"))
				gpsClient.m_serial.Close();
		}
		else
		{
			if (ImGui::Button("Open"))
			{
				if (gpsClient.m_serial.Open(m_ports[com].first, 9600))
				{
					int a = 0;
				}
			}
		}
	}
	else if (cGpsClient::eInputType::Network == gpsClient.m_inputType)
	{
		if (gpsClient.IsReadyConnect())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
			ImGui::Text("Try Connect...");
			ImGui::PopStyleColor();
		}
		else if (gpsClient.IsConnect())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
			ImGui::Text("Success Connect GPS Server");
			ImGui::PopStyleColor();
		}
		else
		{
			ImGui::Text("GPS Server IP/Port");
			ImGui::InputText("IP", gpsClient.m_ip.m_str, gpsClient.m_ip.SIZE);
			ImGui::InputInt("Port", &gpsClient.m_port);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.6f, 0.f, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0.8f, 0.f, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0.4f, 0.f, 1));
			if (ImGui::Button("Connnect"))
			{
				gpsClient.ConnectGpsServer(gpsClient.m_ip.c_str(), gpsClient.m_port);
			}
			ImGui::PopStyleColor(3);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	auto &track = g_global->m_gpsClient.m_paths;
	ImGui::Text("Path Count = %d", track.size());
	ImGui::Text("GPS Count = %d", g_global->m_gpsClient.m_recvCount);
	ImGui::Checkbox("Quadtree", &terrain.m_isShowQuadTree);
	ImGui::SameLine();
	ImGui::Checkbox("Facility", &terrain.m_isShowFacility);
	ImGui::Checkbox("Level", &terrain.m_isShowLevel);
	ImGui::SameLine();
	ImGui::Checkbox("Locs", &terrain.m_isShowLoc);
	ImGui::Checkbox("Poi1", &terrain.m_isShowPoi1);
	ImGui::SameLine();
	ImGui::Checkbox("Poi2", &terrain.m_isShowPoi2);
	ImGui::Checkbox("GPS", &g_global->m_isShowGPS);
	ImGui::SameLine();
	ImGui::Checkbox("Trace GPS", &g_global->m_isTraceGPSPos);
	if (ImGui::Checkbox("Optimize Render", &terrain.m_isRenderOptimize))
		if (!terrain.m_isRenderOptimize)
			terrain.m_optimizeLevel = cQuadTree<sQuadData>::MAX_LEVEL;

	ImGui::Spacing();

	ImGui::Separator();
	ImGui::Spacing();

	// Information
	ImGui::Text("tile memory %d", terrain.m_tileMgr.m_tiles.size());
	ImGui::Text("render quad lv %d", terrain.m_optimizeLevel);
	ImGui::Text("download %d"
		, terrain.m_tileMgr.m_vworldDownloader.m_requestIds.size());
	ImGui::Text("    - total size %I64d (MB)"
		, terrain.m_tileMgr.m_vworldDownloader.m_totalDownloadFileSize / (1048576)); // 1024*1024


	ImGui::Spacing();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0, 1));
	if (ImGui::Button("Jump to Current Position"))
	{
		auto &mapView = viewer->m_mapView;
		auto &camera = mapView->m_camera;
		const Vector3 lookAt = mapView->m_quadTree.Get3DPos(mapView->m_curGpsPos);
		const Vector3 eyePos = lookAt + Vector3(1, 1, 1) * 250.f;
		camera.Move(eyePos, lookAt);

	}
	ImGui::PopStyleColor(3);
	ImGui::Spacing();

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.1f, 0, 1));
	if (ImGui::Button("Clear Path"))
	{
		auto &track = g_global->m_gpsClient.m_paths;
		track.clear();
	}

	if (ImGui::Button("Read Path File"))
	{
		if (g_global->m_gpsClient.ReadPathFile("path.txt"))
		{
		}
	}

	if (ImGui::Button("GPS Path Pump"))
	{
		if (g_global->m_gpsClient.ReadPathFile("path.txt"))
		{
			g_global->m_gpsClient.FileReplay();
		}
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();

	ImGui::RadioButton("MapView", (int*)&g_global->m_analysisType, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Terrain", (int*)&g_global->m_analysisType, 1);
	ImGui::RadioButton("Main", (int*)&g_global->m_analysisType, 2);

	switch (g_global->m_analysisType)
	{
	case eAnalysisType::MapView:
		//ImGui::Text("t0 = %f", g_global->m_renderT0);
		//ImGui::Text("t1 = %f", g_global->m_renderT1);
		//ImGui::Text("t0 + t1 = %f"
		//	, g_global->m_renderT0 + g_global->m_renderT1);
		break;

	case eAnalysisType::Terrain:
		//ImGui::Text("t0 = %f", viewer->m_mapView->m_quadTree.m_t0);
		//ImGui::Text("t1 = %f", viewer->m_mapView->m_quadTree.m_t1);
		//ImGui::Text("t0 + t1 = %f"
		//	, viewer->m_mapView->m_quadTree.m_t0 + viewer->m_mapView->m_quadTree.m_t1);
		break;

	case eAnalysisType::GMain:
		//ImGui::Text("t0 = %f", g_application->m_deltaSeconds);
		break;

	default: assert(0); break;
	}
	ImGui::Checkbox("Show Terrain", &g_global->m_isShowTerrain);
	ImGui::Checkbox("Show MapView", &g_global->m_isShowMapView);
	ImGui::Checkbox("Show Render Overhead", &g_global->m_isShowRenderGraph);
	ImGui::Checkbox("Calc Render Overhead", &g_global->m_isCalcRenderGraph);
	
	ImGui::PopStyleColor(3);
}
