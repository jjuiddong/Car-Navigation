
#include "stdafx.h"
#include "navigationview.h"
#include "worldviewer.h"
#include "mapview.h"

using namespace graphic;
using namespace framework;


cNavigationView::cNavigationView(const StrId &name)
	: framework::cDockWindow(name)
{
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
	ImGui::Text(IsTouchWindow(m_owner->getSystemHandle(), NULL) ? "Touch Mode" : "Gesture Mode");
	ImGui::PopStyleColor();

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
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Path Count = %d", g_root.m_track.size());
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
	ImGui::Checkbox("GPS", &g_root.m_isShowGPS);
	ImGui::SameLine();
	ImGui::Checkbox("Trace GPS", &g_root.m_isTraceGPSPos);
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
		g_root.m_track.clear();
	}

	if (ImGui::Button("Read Path File"))
	{
		if (g_global->m_gpsClient.ReadPathFile("path.txt"))
		{
			g_root.m_track.clear();
			g_root.m_track.reserve(g_global->m_gpsClient.m_paths.size());
			for (auto &p : g_global->m_gpsClient.m_paths)
			{
				const Vector3 pos = terrain.Get3DPos(p.lonLat);
				g_root.m_track.push_back(pos);
			}
		}
	}

	if (ImGui::Button("GPS Path Pump"))
	{
		if (g_global->m_gpsClient.ReadPathFile("path.txt"))
		{
			g_root.m_track.clear();
			g_global->m_gpsClient.FileReplay();
		}
	}

	ImGui::PopStyleColor(3);
}
