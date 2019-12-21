
#include "stdafx.h"
#include "taskwebdownload.h"
#include <direct.h> //mkdir
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")


using namespace gis;


cTaskWebDownload::cTaskWebDownload()
	: cTask(common::GenerateId(), "cTaskWebDownload", true)
{
}

cTaskWebDownload::cTaskWebDownload(cGeoDownloader *webDownloader
	, cQuadTileManager *tileMgr
	, const sDownloadData &dnData)
	: cTask(common::GenerateId(), "cTaskWebDownload", true)
{
	SetParameter(webDownloader, tileMgr, dnData);
}

cTaskWebDownload::~cTaskWebDownload() 
{
	Clear();
}

void cTaskWebDownload::Clear()
{
	if (m_webDownloader)
		m_webDownloader->Remove(m_dnData);
}


void cTaskWebDownload::SetParameter(cGeoDownloader *webDownloader
		, cQuadTileManager *tileMgr
		, const sDownloadData &dnData) 
{
	m_webDownloader = webDownloader;
	m_dnData = dnData;
	m_tileMgr = tileMgr;
}


common::cTask::eRunResult::Enum cTaskWebDownload::Run(const double deltaSeconds)
{
	// 해당 타일이 지워진 상태라면 다운로드 받지 않는다.
	if (!m_tileMgr->FindTile(m_dnData.level, m_dnData.xLoc, m_dnData.yLoc))
	{
		//dbg::Logp("skip web download level=%d, xLoc=%d, yLoc=%d\n", m_dnData.level, m_dnData.xLoc, m_dnData.yLoc);
		m_webDownloader->Remove(m_dnData);
		return eRunResult::END;
	}

	//sample "http://xdworld.vworld.kr:8080/XDServer/requestLayerNode?APIKey=A3C8B7D2-1149-3C99-9862-04D6C24730E9&Layer=dem&Level=7&IDX=1091&IDY=452";

	const char *apiKey = "A3C8B7D2-1149-3C99-9862-04D6C24730E9";
	const char *site = "http://xdworld.vworld.kr:8080/XDServer/";
	const char *cmd = NULL;
	const char *layerName = NULL;
	const char *dir = g_mediaDir.c_str();
	const StrPath dstFileName = gis::GetDownloadFileName(g_mediaDir, m_dnData);

	switch (m_dnData.layer)
	{
	case eLayerName::DEM:
		//cmd = "requestLayerNode";
		//layerName = "dem";
		//break;
		m_webDownloader->Remove(m_dnData);
		return eRunResult::END;

	case eLayerName::TILE:
		//cmd = "requestLayerNode";
		//layerName = "tile";
		//break;
		m_webDownloader->Remove(m_dnData);
		return eRunResult::END;

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
		m_webDownloader->Remove(m_dnData);
		return eRunResult::END;

	case eLayerName::FACILITY_BUILD_GET:
		//cmd = "requestLayerObject";
		//layerName = "facility_build";
		//break;
		m_webDownloader->Remove(m_dnData);
		return eRunResult::END;

	case eLayerName::FACILITY_BUILD_GET_JPG:
		//cmd = "requestLayerObject";
		//layerName = "facility_build";
		//break;
		m_webDownloader->Remove(m_dnData);
		return eRunResult::END;

	default: assert(0); break;
	}

	if (!cmd)
	{
		// 더이상 쓸수 없는 API일 경우 종료
		m_webDownloader->Remove(m_dnData);
		return eRunResult::END;
	}

	Str256 url;
	url.Format("%s%s?APIKey=%s&Layer=%s&Level=%d&IDX=%d&IDY=%d", site, cmd, apiKey, layerName
		, m_dnData.level, m_dnData.xLoc, m_dnData.yLoc);

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

		dirPath.Format("%s\\%d\\%04d", dir, m_dnData.level, m_dnData.yLoc);
		if (!common::IsFileExist(dirPath))
			_mkdir(dirPath.c_str());
	}

	const HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), dstFileName.c_str(), 0, NULL);
	if (S_OK == hr)
		m_webDownloader->Insert(m_dnData);
	else
		m_webDownloader->Remove(m_dnData);

	//if (S_OK == hr)
	//	dbg::Logp("success web download url = [%s]\n", url.c_str());
	//else
	//	dbg::Logp("error web download url = [%s]\n", url.c_str());

	return eRunResult::END;
}
