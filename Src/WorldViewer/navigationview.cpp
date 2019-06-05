
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
	cTerrainQuadTree &terrain = viewer->m_simView->m_quadTree;

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
	ImGui::Text(IsTouchWindow(m_owner->getSystemHandle(), NULL) ? "Touch Mode" : "Gesture Mode");
	ImGui::PopStyleColor();

	if (g_global->m_gpsClient.IsReadyConnect())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
		ImGui::Text("Try Connect...");
		ImGui::PopStyleColor();
	}
	else if (g_global->m_gpsClient.IsConnect())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
		ImGui::Text("Success Connect GPS Server");
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::Text("GPS Server IP/Port");
		ImGui::InputText("IP", g_global->m_gpsIp.m_str, g_global->m_gpsIp.SIZE);
		ImGui::InputInt("Port", &g_global->m_gpsPort);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.6f, 0.f, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0.8f, 0.f, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0.4f, 0.f, 1));
		if (ImGui::Button("Connnect"))
		{
			g_global->m_gpsClient.Init(g_global->m_gpsIp.c_str(), g_global->m_gpsPort);
		}
		ImGui::PopStyleColor(3);
	}
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Path Count = %d", g_root.m_track.size());
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

	ImGui::Spacing();

	ImGui::Separator();
	ImGui::Spacing();

	// Information
	ImGui::Text("tile memory %d", terrain.m_tileMgr.m_tiles.size());
	ImGui::Text("hmap fileloader");
	ImGui::Text("    - file count = %d", terrain.m_tileMgr.m_hmaps.m_files.size());
	ImGui::Text("    - task = %d", terrain.m_tileMgr.m_hmaps.m_tpLoader.m_tasks.size());
	ImGui::Text("tmap fileloader");
	ImGui::Text("    - file count = %d", terrain.m_tileMgr.m_tmaps.m_files.size());
	ImGui::Text("    - task = %d", terrain.m_tileMgr.m_tmaps.m_tpLoader.m_tasks.size());
	ImGui::Text("poi_base fileloader");
	ImGui::Text("    - file count = %d", terrain.m_tileMgr.m_pmaps[0].m_files.size());
	ImGui::Text("    - task = %d", terrain.m_tileMgr.m_pmaps[0].m_tpLoader.m_tasks.size());
	ImGui::Text("poi_bound fileloader");
	ImGui::Text("    - file count = %d", terrain.m_tileMgr.m_pmaps[1].m_files.size());
	ImGui::Text("    - task = %d", terrain.m_tileMgr.m_pmaps[1].m_tpLoader.m_tasks.size());
	ImGui::Text("xdo model");
	ImGui::Text("    - file count = %d", terrain.m_tileMgr.m_facilities.m_files.size());
	ImGui::Text("    - task = %d", terrain.m_tileMgr.m_facilities.m_tpLoader.m_tasks.size());
	ImGui::Text("xdo texture");
	ImGui::Text("    - file count = %d", terrain.m_tileMgr.m_facilitiesTex.m_files.size());
	ImGui::Text("    - task = %d", terrain.m_tileMgr.m_facilitiesTex.m_tpLoader.m_tasks.size());
	ImGui::Text("model");
	ImGui::Text("	- file count = %d", cResourceManager::Get()->m_assimpModels.size());
	ImGui::Text("	- read load file count = %d", terrain.m_tileMgr.m_readyLoadModel.size());

	ImGui::Text("    - download requestIds %d"
		, terrain.m_tileMgr.m_vworldDownloader.m_requestIds.size());


	ImGui::Spacing();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0, 1));
	if (ImGui::Button("Jump3"))
	{
		const Vector2d lonLat(128.46845f, 37.17420f);
		const Vector3 globalPos = gis::WGS842Pos(lonLat);
		const Vector3 relPos = gis::GetRelationPos(globalPos);

		Vector3 dir = Vector3(2578.13232f, 0.000000000f, 1812.11475f) - Vector3(2886.70898f, 10646.5088f, -3660.70337f);
		dir.Normalize();

		viewer->m_simView->m_camera.Move(
			Vector3(2886.70898f, 10646.5088f, -3660.70337f)
			, Vector3(2578.13232f, 0.000000000f, 1812.11475f)
		);

		const float h = viewer->m_simView->m_quadTree.GetHeight(Vector2(relPos.x, relPos.z));

		viewer->m_simView->m_camera.MoveNext(
			Vector3(relPos.x, h, relPos.z) + dir * -50.f
			, relPos
		);
	}
	ImGui::PopStyleColor(3);
	ImGui::Spacing();

}
