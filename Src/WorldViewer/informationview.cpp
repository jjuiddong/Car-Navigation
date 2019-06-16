
#include "stdafx.h"
#include "informationview.h"
#include "carnavi.h"
#include "mapview.h"

using namespace graphic;
using namespace framework;


cInformationView::cInformationView(const StrId &name)
	: framework::cDockWindow(name)
	, m_fogColor(221.f/255.f, 238.f/255.f, 255.f/255.f)
	, m_fogDistanceRate(30.f)
{
}

cInformationView::~cInformationView()
{
}


void cInformationView::OnUpdate(const float deltaSeconds)
{
}


void cInformationView::OnRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	cViewer *viewer = (cViewer*)g_application;
	cTerrainQuadTree &terrain = viewer->m_mapView->m_quadTree;

	if (ImGui::CollapsingHeader("MapView Information", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Wireframe", &viewer->m_mapView->m_showWireframe);
		ImGui::SameLine();
		ImGui::Checkbox("Quadtree", &terrain.m_isShowQuadTree);
		ImGui::SameLine();
		ImGui::Checkbox("Facility", &terrain.m_isShowFacility);
		ImGui::Checkbox("Level", &terrain.m_isShowLevel);
		ImGui::SameLine();
		ImGui::Checkbox("Locs", &terrain.m_isShowLoc);
		ImGui::SameLine();
		ImGui::Checkbox("Poi1", &terrain.m_isShowPoi1);
		ImGui::SameLine();
		ImGui::Checkbox("Poi2", &terrain.m_isShowPoi2);

		ImGui::InputInt("RenderFlag", &g_root.m_renderFlag);

		static float editAngle = 0.01f;
		ImGui::DragFloat("edit angle", &editAngle, 0.01f);

		// 위경도 출력
		ImGui::Text("WGS84 lat,lon = %f, %f", g_root.m_lonLat.y, g_root.m_lonLat.x);
		ImGui::Text("UTM x,y = %f, %f", g_root.m_utmLoc.x, g_root.m_utmLoc.y);
		ImGui::Spacing();
		ImGui::Spacing();


		bool isEdit = false;
		if (ImGui::DragFloat2("offset lonlat", (float*)&g_root.m_offsetLonLat))
			isEdit = true;
		if (ImGui::DragFloat("y angle", &g_root.m_angleY, 0.1f))
			isEdit = true;

		float h = 0;
		if (ImGui::Button("+h"))
		{
			isEdit = true;
			h = g_root.m_offsetY;
		}
		ImGui::SameLine();
		if (ImGui::Button("-h"))
		{
			isEdit = true;
			h = -g_root.m_offsetY;
		}

		if (isEdit)
		{
			//g_root.m_offsetRot *= q;

			for (auto &kv : g_root.m_mapView->m_quadTree.m_tileMgr.m_tiles)
			{
				if (!kv.second.tile)
					continue;

				cQuadTile *tile = kv.second.tile;
				for (auto &mod : tile->m_mods)
				{
					//tile->m_mod.m_transform.rot = tile->m_facility->GetRotation(g_root.m_offsetLonLat);

					Quaternion q;
					q.SetRotationY(ANGLE2RAD(g_root.m_angleY));
					mod.m_transform.rot = q;

					//tile->m_mod.m_transform.rot *= q;
					mod.m_transform.pos.y += h;
				}
			}
		}

		ImGui::Separator();
		ImGui::Text("Scan Range");
		ImGui::DragFloat("Scan Speed (m/s)", &g_global->m_scanSpeed, 1.f, 0.f, 100000);
		ImGui::DragFloat("Scan Line Offset", &g_global->m_scanLineOffset, 1.f, 0.f, 100000);
		ImGui::Checkbox("Pick Left Top", &g_global->m_pickScanLeftTop);
		ImGui::SameLine();
		ImGui::Checkbox("Pick Right Bottom", &g_global->m_pickScanRightBottom);
		ImGui::DragScalarN("Left Top", ImGuiDataType_Double
			, (double*)&g_global->m_scanLeftTop, 2, 0.001f);
		ImGui::DragScalarN("Right Bottom", ImGuiDataType_Double
			, (double*)&g_global->m_scanRightBottom, 2, 0.001f);

		if (ImGui::Button("Map Scanning"))
		{
			g_global->m_isMapScanning = true;
			g_global->m_isMoveRight = false;

			//const Vector2d lonLat(128.46845f, 37.17420f);
			const Vector2d lonLat(125.0f, 38.00f);
			const Vector3 globalPos = gis::WGS842Pos(lonLat);
			const Vector3 relPos = gis::GetRelationPos(globalPos);
			const float h = viewer->m_mapView->m_quadTree.GetHeight(Vector2(relPos.x, relPos.z));
			viewer->m_mapView->m_camera.MoveNext(
				Vector3(relPos.x, h, relPos.z) + Vector3(1,1,1) * 50.f
				, relPos
				, 1000
			);

			//viewer->m_simView->m_camera.Move(
			//	Vector3(2886.70898f, 10646.5088f, -3660.70337f)
			//	, Vector3(2578.13232f, 0.000000000f, 1812.11475f)
			//);
		}
		ImGui::Separator();


		ImGui::DragFloat4("offset rot", (float*)&g_root.m_offsetRot, 0.00001f);
		ImGui::DragFloat("scaleY", &g_root.m_scaleY, 0.001f);
		ImGui::DragFloat("offsetY", &g_root.m_offsetY, 0.001f);
		ImGui::DragFloat("scale", &g_root.m_scale, 0.001f);

		ImGui::ColorEdit3("Text Color", (float*)&terrain.m_txtColor);
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

		//char *layerName[] = { "DEM", "TILE", "POI_BASE", "POI_BOUND", "FACILITY_BUILD"
		//	, "FACILITY_BUILD_GET", "FACILITY_BUILD_GET_JPG" };
		//for (int i = 0; i < gis::eLayerName::MAX_LAYERNAME; ++i)
		//{
		//	ImGui::Text("%s", layerName[i]);
		//	ImGui::Text("    - download thread task %d"
		//		 , terrain.m_tileMgr.m_vworldDownloader.m_webDownloader[i]->thread.GetTaskCount());
		//	ImGui::Text("    - download requestIds %d"
		//		, terrain.m_tileMgr.m_vworldDownloader.m_webDownloader[i]->requestIds.size());
		//}

		ImGui::Text("    - download requestIds %d"
			, terrain.m_tileMgr.m_vworldDownloader.m_requestIds.size());

		//ImGui::ColorEdit3("Fog Color", (float*)&m_fogColor);
		//ImGui::DragFloat("Fog Distance Rate", &m_fogDistanceRate, 0.1f, 1, 1000);

		XMFLOAT4 fog;
		fog.x = m_fogColor.x;
		fog.y = m_fogColor.y;
		fog.z = m_fogColor.z;
		fog.w = 1 / (viewer->m_mapView->m_camera.GetEyePos().y * m_fogDistanceRate);
		renderer.m_cbPerFrame.m_v->fogColor = XMLoadFloat4(&fog);
	}
}
