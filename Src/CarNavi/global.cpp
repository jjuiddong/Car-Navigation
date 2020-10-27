
#include "stdafx.h"
#include "global.h"
#include <limits>


cGlobal::cGlobal()
	: m_isMapScanning(false)
	, m_scanType(0)
	, m_scanPathIdx(0)
	, m_isSelectMapScanningCenter(false)
	, m_isShowGPS(true)
	, m_isTraceGPSPos(true)
	, m_isRotateTrace(true)
	, m_isShowTerrain(true)
	, m_isShowRenderGraph(false)
	, m_isCalcRenderGraph(false)
	, m_analysisType(eAnalysisType::MapView)
	, m_renderT0(0.f)
	, m_renderT1(0.f)
	, m_renderT2(0.f)
	, m_isShowMapView(true)
	, m_scanRadius(1.f)
	, m_scanHeight(100.f)
	, m_scanSpeed(100.f)
	, m_prevDistance(FLT_MAX)
	, m_isMakeScanPath(false)
	, m_isShowPrevPath(false)
	, m_isShowLandMark(true)
	, m_isShowLandMark2(true)
	, m_landMarkSelectState(0)
	, m_landMarkSelectState2(0)
	, m_rpm(0)
	, m_speed(0)
	, m_gear(0)
	, m_obdRcvCnt(0)
	, m_isDebugMode(false)
	, m_isDarkMode(false)
	, m_darkColor(0.1f, 0.1f, 0.1f, 1.f)
	, m_camType(eCameraType::Custom)
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
	g_mediaDir = m_config.GetString("media_path", "D:\\media\\data");

	// read camera information
	for (int i = 0; i < (int)eCameraType::MAX; ++i)
	{
		string key1 = common::format("view_camera%d_lookat_y", i);
		string key2 = common::format("view_camera%d_distance", i);
		m_camInfo[i].lookAtY = m_config.GetFloat(key1, -0.5f);
		m_camInfo[i].distance = m_config.GetFloat(key2, 30.f);
	}

	const float darkColor = m_config.GetFloat("darkcolor", 0.1f);
	m_darkColor = Vector4(darkColor, darkColor, darkColor, 1.f);

	m_timer.Create();
	m_touch.Init(hwnd);
	m_gpsClient.Init();
	m_landMark.Read("landmark.txt");

	// ��¥ ������ path ��� �α׸� �����Ѵ�.
	int fileId = 0;
	do
	{
		m_pathFilename = "./path/path_";
		m_pathFilename += common::GetCurrentDateTime4();
		if (fileId > 0)
			m_pathFilename += common::format("-%d", fileId);
		m_pathFilename += ".txt";
		++fileId;
	} while (m_pathFilename.IsFileExist());

	//m_shape.Read("./media/road/Z_UPIS_C_UQ1512.shp");

	return true;
}


// pathDirectoryName ���� �ִ� *.txt ���� (path ����)�� �о
// ȭ�鿡 ǥ���Ѵ�. ���� ������ ��� *.3dpos ���Ϸ� ��ȯ�ؼ� �����Ѵ�.
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
		cPathFile path(file);
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


// pathDirectoryName ���� �ִ� *.3dpos ���� (path ����)�� �о
// ȭ�鿡 ǥ���Ѵ�. 
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


// pathDirectoryName ���� ���� *.txt path������ �о�´�.
// path���Ͽ� �ش��ϴ� *.3dpos ������ �ִٸ�, �ش� ������ �о� �´�.
// path���� - 3dpos ������ ���ϸ��� ����, Ȯ���ڸ� �ٸ���.
// path���� �߿� 3dpos ���Ϸ� ��ȯ���� �ʴ� ������ ��ȯ��Ų��.
bool cGlobal::ReadAndConvertPathFiles(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain
	, const StrPath &pathDirectoryName)
{
	list<string> files;
	list<string> exts;
	exts.push_back(".txt");
	common::CollectFiles(exts, pathDirectoryName.c_str(), files);

	m_pathRenderers.reserve(files.size());
	const string curPathFileName = m_pathFilename.GetFileName();

	for (auto &file : files)
	{
		// ���� �ǽð����� ������Ʈ �ǰ� �ִ� pathFile�� ���� �ʴ´�.
		if (curPathFileName == StrPath(file).GetFileName())
			continue;

		StrPath pos3DFileName = StrPath(file).GetFileNameExceptExt2();
		pos3DFileName += ".3dpos";

		auto it = std::find_if(m_pathRenderers.begin(), m_pathRenderers.end()
			, [&](const auto &a) { return a->m_fileName == pos3DFileName; });
		if (m_pathRenderers.end() != it)
			continue; // already loaded

		cPathRenderer *p = NULL;
		if (pos3DFileName.IsFileExist())
		{
			// read *.3dpos file
			p = new cPathRenderer();
			if (!p->Create(renderer, pos3DFileName))
			{
				delete p;
				continue; // error occurred
			}
		}
		else
		{
			cPathFile path(file);
			if (!path.IsLoad())
				continue; // error occurred

			// read *.txt file
			p = new cPathRenderer();
			if (!p->Create(renderer, terrain, path, pos3DFileName))
			{
				delete p; 
				continue; // error occurred
			}
		}

		terrain.Clear();
		m_pathRenderers.push_back(p);
	}
	return true;
}


// binary �������ε� trackpos ������ path �������·� �����Ѵ�.
bool cGlobal::ConvertTrackPos2Path()
{
	typedef std::numeric_limits< double > dbl;

	list<string> files;
	list<string> exts;
	exts.push_back(".txt");
	common::CollectFiles(exts, "./TrackPos_LittleEndian", files);

	for (auto &file : files)
	{
		std::ifstream ifs(file, std::ios::binary);
		if (!ifs.is_open())
			continue;
		
		StrPath outFileName = "./path2/";
		outFileName += StrPath(file).GetFileName();

		std::ofstream ofs(outFileName.c_str());
		if (!ofs.is_open())
			continue;
		ofs.precision(dbl::max_digits10);

		while (!ifs.eof())
		{
			Vector2d pos(0, 0);
			ifs.read((char*)&pos.y, sizeof(pos.y));
			ifs.read((char*)&pos.x, sizeof(pos.x));

			if ((pos.x == 0.f) || (pos.y == 0))
				continue;

			//ex) 2019-06-30-14-47-23-301, 126.833816528320313, 37.617435455322266
			ofs << "2018-06-30-14-47-23-301, ";
			ofs << pos.x << ", " << pos.y << std::endl;
		}
	}

	return true;
}


void cGlobal::Recv(const int pid, const int data)
{
	++m_obdRcvCnt;
	switch ((cOBD2::ePID)pid)
	{
	case cOBD2::PID_RPM: m_rpm = data; break;
	case cOBD2::PID_SPEED: m_speed = data; break;
	case cOBD2::PID_TRANSMISSION_GEAR: m_gear = data; break;
	}
}


void cGlobal::Clear()
{
	for (auto &p : m_pathRenderers)
		delete p;
	m_pathRenderers.clear();
}
