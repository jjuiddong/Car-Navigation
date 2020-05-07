//
// Car Navigation
//
#include "stdafx.h"
#include "mapview.h"
#include "carnavi.h"
#include "informationview.h"
#include "navigationview.h"

using namespace graphic;
using namespace framework;

INIT_FRAMEWORK3(cViewer);

common::cMemoryPool2<65 * 65 * sizeof(float)> g_memPool65;
common::cMemoryPool2<67 * 67 * sizeof(float)> g_memPool67;
common::cMemoryPool2<256 * 256 * sizeof(float)> g_memPool256;
common::cMemoryPool2<258 * 258 * sizeof(float)> g_memPool258;
common::cMemoryPool3<graphic::cTexture, 512> g_memPoolTex;

cRoot g_root;
cGlobal *g_global = NULL;


cViewer::cViewer()
{
	m_windowName = L"Navigation";
	m_isLazyMode = true;
	const RECT r = { 0, 0, 1024, 768 };
	//const RECT r = { 0, 0, 1280, 960 };
	m_windowRect = r;
	graphic::cResourceManager::Get()->SetMediaDirectory("./media/");
}

cViewer::~cViewer()
{
	SAFE_DELETE(g_global);
}


bool cViewer::OnInit()
{
	dbg::RemoveLog();
	dbg::RemoveErrLog();

	const float WINSIZE_X = float(m_windowRect.right - m_windowRect.left);
	const float WINSIZE_Y = float(m_windowRect.bottom - m_windowRect.top);
	GetMainCamera().SetCamera(Vector3(30, 30, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
	GetMainCamera().SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 0.1f, 10000.0f);
	GetMainCamera().SetViewPort(WINSIZE_X, WINSIZE_Y);

	m_camera.SetCamera(Vector3(-3, 10, -10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL,
		Vector4(0.2f, 0.2f, 0.2f, 1), Vector4(0.9f, 0.9f, 0.9f, 1),
		Vector4(0.2f, 0.2f, 0.2f, 1));
	const Vector3 lightPos(-300, 300, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	m_gui.SetContext();

	g_global = new cGlobal();
	g_global->Init(getSystemHandle());

	m_mapView = new cMapView("Map View");
	m_mapView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, NULL);
	bool result = m_mapView->Init(m_renderer);
	assert(result);

	m_naviView = new cNavigationView("Navigation View");
	m_naviView->m_owner = this;

	//m_infoView = new cInformationView("Information View");
	//m_infoView->Create(eDockState::DOCKWINDOW, eDockSlot::RIGHT, this, m_mapView);

	//m_observerView = new cObserverView("Observer View");
	//m_observerView->Create(eDockState::DOCKWINDOW, eDockSlot::BOTTOM, this, m_simView, 0.3f);// 0.6f, framework::eDockSizingOption::PIXEL);
	//result = m_observerView->Init(m_renderer);
	//assert(result);

	g_root.m_mapView = m_mapView;
	g_global->m_mapView = m_mapView;
	g_global->m_infoView = m_infoView;
	g_global->m_naviView = m_naviView;

	if (g_global->m_isShowPrevPath)
		g_global->ReadAndConvertPathFiles(m_renderer, m_mapView->m_quadTree, "./path/");

	m_gui.SetContext();
	m_gui.SetStyleColorsDark();

	//ShowWindow(getSystemHandle(), SW_MAXIMIZE);
	//g_global->ConvertTrackPos2Path();

	//double totDistance = 0.f;
	//cPath path;
	//if (path.Read("path/path_20200424.txt"))
	//{
	//	for (uint i = 1; i < path.m_table.size(); ++i)
	//	{
	//		cPath::sRow r0 = path.m_table[i - 1];
	//		cPath::sRow r1 = path.m_table[i];
	//		const double d = gis::WGS84Distance2(r0.lonLat, r1.lonLat);
	//		totDistance += d;
	//	}
	//}

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	__super::OnUpdate(deltaSeconds);
	cAutoCam cam(&m_camera);
	GetMainCamera().Update(deltaSeconds);
}


void cViewer::OnRender(const float deltaSeconds)
{
}


void cViewer::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		switch (evt.key.cmd) {
		case sf::Keyboard::Escape: close(); break;
		}
		break;
	}
}
