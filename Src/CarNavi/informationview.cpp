
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
		// 위경도 출력
		ImGui::Text("WGS84 lat,lon = %f, %f", g_root.m_lonLat.y, g_root.m_lonLat.x);
		ImGui::Text("UTM x,y = %f, %f", g_root.m_utmLoc.x, g_root.m_utmLoc.y);

		ImGui::Separator();
		ImGui::Text("Scanning Information");

		if (!g_global->m_isSelectMapScanningCenter)
		{
			if (ImGui::Button("Set Scanning Center"))
			{
				g_global->m_isSelectMapScanningCenter = true;
			}
		}
		else
		{
			ImGui::Text("Select Scanning Center");
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				g_global->m_isSelectMapScanningCenter = false;
			}

		}

		ImGui::DragScalarN("Left Top", ImGuiDataType_Double
			, (double*)&g_global->m_scanCenter, 2, 0.001f);
		ImGui::DragFloat("Scan Height", &g_global->m_scanHeight, 0.001f);
		ImGui::DragFloat("Scan Speed", &g_global->m_scanSpeed, 0.001f);

		if (g_global->m_isMapScanning)
		{
			if (ImGui::Button("Cancel Map Scanning"))
			{
				g_global->m_isMapScanning = false;
			}
		}
		else
		{
			if (ImGui::Button("Map Scanning"))
			{
				g_global->m_isMapScanning = true;
				Vector3 center = terrain.Get3DPos(g_global->m_scanCenter);
				g_global->m_scanRadius = 10.f;
				g_global->m_scanPos = center + Vector3(10, 0, 0);
				g_global->m_scanCenterPos = center;
				g_global->m_scanDir = Vector3(0, 1, 0).CrossProduct(g_global->m_scanPos).Normal();
			}
		}
		ImGui::Separator();

		ImGui::Text("download %d"
			, terrain.m_tileMgr->m_geoDownloader.m_requestIds.size());
		ImGui::Text("    - total size %I64d (MB)"
			, terrain.m_tileMgr->m_geoDownloader.m_totalDownloadFileSize / (1048576)); // 1024*1024
		//ImGui::Text("load texture %d"
		//	, terrain.m_tileMgr->m_tmaps.m_tpLoader.m_tasks.size());

		if (g_global->m_isMakeTracePath)
		{
			if (ImGui::Button("End Make Trace Line"))
			{
				g_global->m_isMakeTracePath = false;
			}
		}
		else
		{
			if (ImGui::Button("Make Trace Line"))
			{
				g_global->m_isMakeTracePath = true;
			}
		}

		if (g_global->m_gpsClient.IsPathReplay())
		{
			if (ImGui::Button("End File Trace"))
			{
				g_global->m_gpsClient.StopPathFileReplay();
			}
		}
		else
		{
			if (ImGui::Button("Start File Trace"))
			{
				g_global->m_gpsClient.PathFileReplay();
				g_global->m_prevTracePos = Vector3::Zeroes;
			}
		}

		//XMFLOAT4 fog;
		//fog.x = m_fogColor.x;
		//fog.y = m_fogColor.y;
		//fog.z = m_fogColor.z;
		//fog.w = 1 / (viewer->m_mapView->m_camera.GetEyePos().y * m_fogDistanceRate);
		//renderer.m_cbPerFrame.m_v->fogColor = XMLoadFloat4(&fog);
	}
}
