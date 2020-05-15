
#include "stdafx.h"
#include "navigationview.h"
#include "carnavi.h"
#include "mapview.h"

using namespace graphic;
using namespace framework;


cNavigationView::cNavigationView(const StrId &name)
	: framework::cDockWindow(name)
	, m_serialConnectTime(0)
	, m_comboIdx(0)
{

	// 시리얼 포트 열거, 콤보 박스 문자열로 변환한다.
	common::EnumCOMPort(m_ports);
	string str;
	for (uint i=0; i < m_ports.size(); ++i)
	{
		const auto &port = m_ports[i];
		StrId id = port.second;
		str += id.utf8().c_str();
		str += "^";

		// find GPS USB Serial Port
		if (id.find("Serial") || id.find("SERIAL"))
			m_comboIdx = i;
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

class AutoFontPop
{
public:
	AutoFontPop(ImFont *font) : m_font(font) { if (font) ImGui::PushFont(font); }
	void PushFont(ImFont *font) { m_font = font;  if (font) ImGui::PushFont(font); }
	virtual ~AutoFontPop() { if (m_font) ImGui::PopFont(); }
	ImFont *m_font;
};


void cNavigationView::OnRender(const float deltaSeconds)
{
	AutoFontPop autoFont(nullptr);
	if (!g_global->m_isDebugMode)
		autoFont.PushFont(m_owner->m_fontBig);

	static bool isDetailOption = false;
	ImGui::Checkbox("Detail Setting", &isDetailOption);

	// camera view selection button
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration 
		| ImGuiWindowFlags_NoBackground
		;
	ImGui::SetNextWindowPos(ImVec2(g_global->m_mapView->m_viewRect.right - 400.f
		, g_global->m_mapView->m_viewRect.bottom - 55.f));
	ImGui::SetNextWindowSize(ImVec2(460, 55));
	if (ImGui::Begin("Camera Setting", nullptr, flags))
	{
		if (ImGui::RadioButton("custom", (int*)&g_global->m_camType, (int)eCameraType::Custom))
			g_global->m_mapView->ChangeViewCamera(eCameraType::Custom);
		ImGui::SameLine();
		if (ImGui::RadioButton("c1", (int*)&g_global->m_camType, (int)eCameraType::Camera1))
			g_global->m_mapView->ChangeViewCamera(eCameraType::Camera1);
		ImGui::SameLine();
		if (ImGui::RadioButton("c2", (int*)&g_global->m_camType, (int)eCameraType::Camera2))
			g_global->m_mapView->ChangeViewCamera(eCameraType::Camera2);
		ImGui::SameLine();
		if (ImGui::RadioButton("c3", (int*)&g_global->m_camType, (int)eCameraType::Camera3))
			g_global->m_mapView->ChangeViewCamera(eCameraType::Camera3);
		ImGui::SameLine();
		if (ImGui::RadioButton("c4", (int*)&g_global->m_camType, (int)eCameraType::Camera4))
			g_global->m_mapView->ChangeViewCamera(eCameraType::Camera4);

		ImGui::End();
	}
	//

	// Show Prev Path CheckBox
	ImGui::SetNextWindowPos(ImVec2(g_global->m_mapView->m_viewRect.left + 150.f
		, g_global->m_mapView->m_viewRect.bottom - 55.f));
	ImGui::SetNextWindowSize(ImVec2(300, 55));
	if (ImGui::Begin("Prev Path Window", nullptr, flags))
	{
		if (ImGui::Checkbox("Show Prev Path", &g_global->m_isShowPrevPath))
		{
			if (g_global->m_isShowPrevPath)
				g_global->ReadAndConvertPathFiles(g_global->m_mapView->GetRenderer()
					, g_global->m_mapView->m_quadTree, "./path/");
			//cPathCompare comp;
			//comp.Compare("./path/");
		}
		//ImGui::SameLine();
		//ImGui::Checkbox("DeepCopy Smooth", &g_global->m_mapView->m_quadTree.m_tileMgr->m_isDeepCopySmooth);
		ImGui::End();
	}
	//


	cTerrainQuadTree &terrain = g_global->m_mapView->m_quadTree;
	cGpsClient &gpsClient = g_global->m_gpsClient;

	if (isDetailOption)
	{
		ImGui::RadioButton("Serial", (int*)&gpsClient.m_inputType
			, (int)cGpsClient::eInputType::Serial);
		ImGui::SameLine();
		ImGui::RadioButton("Network", (int*)&gpsClient.m_inputType
			, (int)cGpsClient::eInputType::Network);
	}
	else
	{
		for (int i=0; i < 8; ++i)
			ImGui::Spacing();
	}

	const int width = 200;
	ImVec2 wndSize = ImVec2(width, 35);
	if (gpsClient.m_inputType == cGpsClient::eInputType::Network)
	{
		if (gpsClient.IsReadyConnect() || gpsClient.IsConnect())
			wndSize = ImVec2(width, 35);
		else
			wndSize = ImVec2(width, 145);
	}

	ImVec4 bgColor = gpsClient.IsConnect()? ImVec4(0, 1, 0, 0.7f) : ImVec4(1, 0, 0, 0.7f);
	if (g_global->m_isDarkMode)
		bgColor.w = 0.1f;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);

	if (ImGui::BeginChild("Input Device", wndSize))
	{
		ImGui::SetCursorPosY(2);
		ImGui::Indent(2);

		if (cGpsClient::eInputType::Serial == gpsClient.m_inputType)
		{
			ImGui::PushItemWidth(70);
			ImGui::Combo("Port", &m_comboIdx, m_comboStr.c_str());
			ImGui::SameLine();
			ImGui::PopItemWidth();

			if (gpsClient.m_serial.IsOpen())
			{
				m_serialConnectTime = 0.f;

				if (ImGui::Button("Close"))
				{
					gpsClient.m_serial.Close();
				}
			}
			else
			{
				// 접속이 끊겼다면, 5초마다 한번씩 접속을 시도한다.
				m_serialConnectTime += deltaSeconds;
				if ((m_serialConnectTime > 5.f)
					&& ((int)m_ports.size() > m_comboIdx))
				{
					gpsClient.ConnectGpsSerial(m_ports[m_comboIdx].first, 9600);
					m_serialConnectTime = 0.f;
				}

				if (ImGui::Button("Open"))
				{
					if ((int)m_ports.size() > m_comboIdx)
					{
						gpsClient.ConnectGpsSerial(m_ports[m_comboIdx].first, 9600);
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
				ImGui::PushItemWidth(100);
				ImGui::InputText("IP", gpsClient.m_ip.m_str, gpsClient.m_ip.SIZE);
				ImGui::PopItemWidth();
				ImGui::PushItemWidth(100);
				ImGui::InputInt("Port", &gpsClient.m_port);
				ImGui::PopItemWidth();

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
		//ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::PopStyleColor(1);

	if (!isDetailOption)
		return;

	ImGui::Separator();

	static bool isShowOption = false;
	ImGui::Checkbox("Option", &isShowOption);

	auto &track = g_global->m_gpsClient.m_paths;

	if (isShowOption)
	{
		ImGui::Text("Path Count = %d", track.size());
		ImGui::Text("GPS Count = %d", g_global->m_gpsClient.m_recvCount);	
		ImGui::Text("la d = %.3f", g_global->m_mapView->m_lookAtDistance);
		ImGui::Text("la y = %.3f", g_global->m_mapView->m_lookAtYVector);
		ImGui::Checkbox("Quadtree", &terrain.m_isShowQuadTree);
		ImGui::SameLine();
		ImGui::Checkbox("Lv", &terrain.m_isShowLevel);
		//ImGui::Checkbox("Poi1", &terrain.m_isShowPoi1);
		//ImGui::SameLine();
		ImGui::Checkbox("Poi2", &terrain.m_isShowPoi2);
		ImGui::SameLine();
		ImGui::Checkbox("GPS", &g_global->m_isShowGPS);

		if (ImGui::Checkbox("Optimize Render", &terrain.m_isRenderOptimize))
			if (!terrain.m_isRenderOptimize)
				terrain.m_optimizeLevel = cQuadTree<sQuadData>::MAX_LEVEL;
	}

	if (!isShowOption)
	{
		ImGui::Checkbox("Trace GPS", &g_global->m_isTraceGPSPos);
		ImGui::Checkbox("Trace Rotate", &g_global->m_isRotateTrace);
		ImGui::Checkbox("Show LandMark", &g_global->m_isShowLandMark);
		ImGui::Checkbox("Show LandMark2", &g_global->m_isShowLandMark2);
		ImGui::Checkbox("Black Mode", &g_global->m_isDarkMode);
		if (g_global->m_isDarkMode)
			if (ImGui::SliderFloat("Alpha", &g_global->m_darkColor.x, 0.f, 1.f))
				g_global->m_darkColor = Vector4(g_global->m_darkColor.x
					, g_global->m_darkColor.x, g_global->m_darkColor.x, g_global->m_darkColor.w);
	}

	//ImGui::Separator();

	// Information
	if (isShowOption)
	{
		ImGui::Text("tile memory %d", terrain.m_tileMgr->m_tiles.size());
		ImGui::Text("render quad lv %d", terrain.m_optimizeLevel);
		//ImGui::Text("download %d"
		//	, terrain.m_tileMgr->m_geoDownloader.m_requestIds.size());
		//ImGui::Text("    - total size %I64d (MB)"
		//	, terrain.m_tileMgr->m_geoDownloader.m_totalDownloadFileSize / (1048576)); // 1024*1024
	}

	if (!isShowOption)
	{
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0, 1));
		if (ImGui::Button("Jump to Current Position"))
		{
			auto &mapView = g_global->m_mapView;
			auto &camera = mapView->m_camera;
			const Vector3 lookAt = mapView->m_quadTree.Get3DPos(mapView->m_curGpsPos);
			const Vector3 eyePos = lookAt + Vector3(1, 1, 1) * 250.f;
			camera.Move(eyePos, lookAt);
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		switch (g_global->m_landMarkSelectState)
		{
		case 0:
			if (ImGui::Button("LandMark"))
				g_global->m_landMarkSelectState = 1;
			break;
		case 1:
		case 2:
			ImGui::Button("Set LandMark");
			break;
		default: assert(0); break;
		}

		ImGui::Spacing();
		ImGui::Spacing();

		switch (g_global->m_landMarkSelectState2)
		{
		case 0:
			if (ImGui::Button("LandMark2"))
				g_global->m_landMarkSelectState2 = 1;
			break;
		case 1:
		case 2:
			ImGui::Button("Set LandMark2");
			break;
		default: assert(0); break;
		}

		ImGui::PopStyleColor(3);
		ImGui::Spacing();

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.1f, 0, 1));
		//if (ImGui::Button("Clear Path"))
		//{
		//	auto &track = g_global->m_gpsClient.m_paths;
		//	track.clear();
		//}

		//if (ImGui::Button("Read Path File"))
		//{
		//	if (g_global->m_gpsClient.ReadPathFile("path.txt"))
		//	{
		//	}
		//}

		//if (ImGui::Button("GPS Path Pump"))
		//{
		//	if (g_global->m_gpsClient.ReadPathFile("path.txt"))
		//	{
		//		g_global->m_gpsClient.FileReplay();
		//	}
		//}
		ImGui::PopStyleColor(3);
	}

	//ImGui::Spacing();
	//ImGui::Spacing();
	//ImGui::Separator();

	static bool isAnalysis = false;
	if (isShowOption)
	{
		ImGui::Checkbox("Analysis Rendering", &isAnalysis);
	}

	if (isShowOption && isAnalysis)
	{
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
	}

}
