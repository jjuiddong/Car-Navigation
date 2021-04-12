
#include "stdafx.h"
#include "taskwebdownload.h"
#include <direct.h> //mkdir
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

using namespace gis;


cTaskWebDownload::cTaskWebDownload(const int id
	, cGeoDownloader *geoDownloader
	, const sDownloadData &dnData)
	: cTask(id, "cTaskWebDownload", true)
	, m_geoDownloader(nullptr)
{
	SetParameter(geoDownloader, dnData);
}

cTaskWebDownload::~cTaskWebDownload() 
{
	Clear();
}

void cTaskWebDownload::Clear()
{
	if (m_geoDownloader)
		m_geoDownloader->FailDownload(m_dnData);
}


void cTaskWebDownload::SetParameter(cGeoDownloader *geoDownloader
		, const sDownloadData &dnData) 
{
	m_geoDownloader = geoDownloader;
	m_dnData = dnData;
}


common::cTask::eRunResult::Enum cTaskWebDownload::Run(const double deltaSeconds)
{
	// �ش� Ÿ���� ������ ���¶�� �ٿ�ε� ���� �ʴ´�.
	//if (m_geoDownloader->m_isCheckTileMgr 
	//	&& !m_tileMgr->FindTile(m_dnData.level, m_dnData.xLoc, m_dnData.yLoc))
	//{
	//	//dbg::Logp("skip web download level=%d, xLoc=%d, yLoc=%d\n", m_dnData.level, m_dnData.xLoc, m_dnData.yLoc);
	//	//m_geoDownloader->Remove(m_dnData);
	//	return eRunResult::END;
	//}

	//sample "http://xdworld.vworld.kr:8080/XDServer/requestLayerNode?APIKey=A3C8B7D2-1149-3C99-XXXX-XXXXXXXXXXXX&Layer=dem&Level=7&IDX=1091&IDY=452";
	const char *apiKey = m_geoDownloader->m_apiKey.c_str();
	const char *site = "http://xdworld.vworld.kr:8080/XDServer/";
	const char *cmd = NULL;
	const char *layerName = NULL;
	const char *dir = g_mediaDir.c_str();
	StrPath dstFileName = gis::GetDownloadFileName(g_mediaDir, m_dnData);

	switch (m_dnData.layer)
	{
	case eLayerName::DEM:
		dir = g_mediaDemDir.c_str();
		cmd = "requestLayerNode";
		layerName = "dem";
		dstFileName = gis::GetDownloadFileName(g_mediaDemDir, m_dnData);
		break;
		//m_geoDownloader->FailDownload(m_dnData);
		//return eRunResult::END;

	case eLayerName::TILE:
		dir = g_mediaTileDir.c_str();
		cmd = "requestLayerNode";
		layerName = "tile";
		dstFileName = gis::GetDownloadFileName(g_mediaTileDir, m_dnData);
		break;
		//m_geoDownloader->FailDownload(m_dnData);
		//return eRunResult::END;

	case eLayerName::POI_BASE:
		cmd = "requestLayerNode";
		layerName = "poi_base";
		break;

	case eLayerName::POI_BOUND:
		cmd = "requestLayerNode";
		layerName = "poi_bound";
		break;

	case eLayerName::FACILITY_BUILD:
		//cmd = "requestLayerNode";
		//layerName = "facility_build";
		//break;
		m_geoDownloader->FailDownload(m_dnData);
		return eRunResult::END;

	case eLayerName::FACILITY_BUILD_GET:
		//cmd = "requestLayerObject";
		//layerName = "facility_build";
		//break;
		m_geoDownloader->FailDownload(m_dnData);
		return eRunResult::END;

	case eLayerName::FACILITY_BUILD_GET_JPG:
		//cmd = "requestLayerObject";
		//layerName = "facility_build";
		//break;
		m_geoDownloader->FailDownload(m_dnData);
		return eRunResult::END;

	default: assert(0); break;
	}

	if (!cmd)
	{
		// no api command? exit
		m_geoDownloader->FailDownload(m_dnData);
		m_geoDownloader = nullptr; // finish
		return eRunResult::END;
	}

	Str256 url;
	url.Format("%s%s?APIKey=%s&Layer=%s&Level=%d&IDX=%d&IDY=%d", site, cmd, apiKey, layerName
		, m_dnData.level, m_dnData.x, m_dnData.y);

	if ((eLayerName::FACILITY_BUILD_GET == m_dnData.layer)
		|| (eLayerName::FACILITY_BUILD_GET_JPG == m_dnData.layer))
	{
		url += "&DataFile=";
		url += m_dnData.dataFile.c_str();
	}

	// make directory (level, yloc directory)
	{
		StrPath dirPath;
		dirPath.Format("%s\\%d", dir, m_dnData.level);
		if (!common::IsFileExist(dirPath))
			_mkdir(dirPath.c_str());

		dirPath.Format("%s\\%d\\%04d", dir, m_dnData.level, m_dnData.y);
		if (!common::IsFileExist(dirPath))
			_mkdir(dirPath.c_str());
	}

	const HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), dstFileName.c_str(), 0, NULL);
	if (S_OK == hr)
		m_geoDownloader->CompleteDownload(m_dnData);
	else
		m_geoDownloader->FailDownload(m_dnData);

	m_geoDownloader = nullptr; // finish
	return eRunResult::END;
}
