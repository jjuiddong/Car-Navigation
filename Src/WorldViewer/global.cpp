
#include "stdafx.h"
#include "global.h"


cGlobal::cGlobal()
	: m_isMapScanning(false)
	, m_isSelectMapScanningCenter(false)
	, m_isShowGPS(true)
	, m_isTraceGPSPos(true)
	, m_isShowTerrain(true)
	, m_isShowRenderGraph(false)
	, m_isCalcRenderGraph(false)
	, m_analysisType(eAnalysisType::MapView)
	, m_renderT0(0.f)
	, m_renderT1(0.f)
	, m_renderT2(0.f)
	, m_isShowMapView(true)
	, m_scanRadius(1.f)
	, m_scanHeight(10.f)
	, m_scanSpeed(1.f)
	, m_isMakeTracePath(false)
	, m_isShowPrevPath(false)
	, m_isShowLandMark(true)
	, m_isShowLandMark2(true)
	, m_landMarkSelectState(0)
	, m_landMarkSelectState2(0)
{
}

cGlobal::~cGlobal()
{
	m_config.Write("carnavi_config.txt");
	m_landMark.Write("landmark.txt");
	Clear();
}


bool cGlobal::Init(HWND hwnd)
{
	m_config.Read("carnavi_config.txt");

	m_timer.Create();
	m_touch.Init(hwnd);
	m_gpsClient.Init();
	m_landMark.Read("landmark.txt");

	return true;
}


// pathDirectoryName 내에 있는 *.txt 파일 (path 파일)을 읽어서
// 화면에 표시한다. 읽은 파일은 모두 *.3dpos 파일로 변환해서 저장한다.
bool cGlobal::ReadPathFiles(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain
	, const StrPath pathDirectoryName)
{
	list<string> files;
	list<string> exts;
	exts.push_back(".txt");
	common::CollectFiles(exts, pathDirectoryName.c_str(), files);

	for (auto &file : files)
	{
		cPath path(file);
		if (!path.IsLoad())
			continue;

		StrPath pos3DFileName = pathDirectoryName;
		pos3DFileName += StrPath(file).GetFileNameExceptExt();
		pos3DFileName += ".3dpos";

		cPathRenderer *p = new cPathRenderer();
		if (!p->Create(renderer, terrain, path, pos3DFileName))
		{
			delete p;
			continue;
		}

		terrain.Clear();
		m_pathRenderers.push_back(p);
	}

	return true;
}


// pathDirectoryName 내에 있는 *.3dpos 파일 (path 파일)을 읽어서
// 화면에 표시한다. 
bool cGlobal::Read3DPosFiles(graphic::cRenderer &renderer, const StrPath pathDirectoryName)
{
	list<string> files;
	list<string> exts;
	exts.push_back(".3dpos");
	common::CollectFiles(exts, pathDirectoryName.c_str(), files);

	for (auto &file : files)
	{
		cPathRenderer *p = new cPathRenderer();
		if (!p->Create(renderer, file))
		{
			delete p;
			continue;
		}

		m_pathRenderers.push_back(p);
	}

	return true;
}


// pathDirectoryName 폴더 안의 *.txt path파일을 읽어온다.
// path파일에 해당하는 *.3dpos 파일이 있다면, 해당 파일을 읽어 온다.
// path파일 - 3dpos 파일은 파일명은 같고, 확장자만 다르다.
// path파일 중에 3dpos 파일로 변환되지 않는 파일은 변환시킨다.
bool cGlobal::ReadAndConvertPathFiles(graphic::cRenderer &renderer, cTerrainQuadTree &terrain
	, const StrPath pathDirectoryName)
{
	list<string> files;
	list<string> exts;
	exts.push_back(".txt");
	common::CollectFiles(exts, pathDirectoryName.c_str(), files);

	for (auto &file : files)
	{
		StrPath pos3DFileName = StrPath(file).GetFileNameExceptExt2();
		pos3DFileName += ".3dpos";

		auto it = std::find_if(m_pathRenderers.begin(), m_pathRenderers.end()
			, [&](const auto &a) { return a->m_fileName == pos3DFileName; });
		if (m_pathRenderers.end() != it)
			continue; // already loaded

		cPathRenderer *p = NULL;
		if (pos3DFileName.IsFileExist())
		{
			p = new cPathRenderer();
			if (!p->Create(renderer, pos3DFileName))
			{
				delete p;
				continue;
			}
		}
		else
		{
			cPath path(file);
			if (!path.IsLoad())
				continue;

			p = new cPathRenderer();
			if (!p->Create(renderer, terrain, path, pos3DFileName))
			{
				delete p;
				continue;
			}
		}

		terrain.Clear();
		m_pathRenderers.push_back(p);
	}
	return true;
}


void cGlobal::Clear()
{
	for (auto &p : m_pathRenderers)
		delete p;
	m_pathRenderers.clear();
}
