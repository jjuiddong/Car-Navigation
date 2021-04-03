
#include "stdafx.h"
#include "taskarcgisdownload.h"
#include <direct.h> //mkdir
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

using namespace gis;


cTaskArcGisDownload::cTaskArcGisDownload(const int id
	, cGeoDownloader *geoDownloader
	, const sDownloadData &dnData)
	: cTask(id, "cTaskArcGisDownload", true)
	, m_geoDownloader(nullptr)
{
	SetParameter(geoDownloader, dnData);
}

cTaskArcGisDownload::~cTaskArcGisDownload()
{
	Clear();
}


void cTaskArcGisDownload::Clear()
{
	if (m_geoDownloader)
		m_geoDownloader->FailDownload(m_dnData);
}


void cTaskArcGisDownload::SetParameter(cGeoDownloader *geoDownloader
	, const sDownloadData &dnData)
{
	m_geoDownloader = geoDownloader;
	m_dnData = dnData;
}


common::cTask::eRunResult::Enum cTaskArcGisDownload::Run(const double deltaSeconds)
{
	//sample "https://services.arcgisonline.com/arcgis/rest/services/Elevation/World_Hillshade/MapServer/tile/0/0/0";
	const char *site = "https://services.arcgisonline.com/ArcGIS/rest/services/";
	const char *cmd = nullptr;
	const char *dir = g_mediaDir.c_str();

	// change original space -> arcgis space
	const int level = m_dnData.level;
	const int x = m_dnData.x;
	const int y = (1 << m_dnData.level) - m_dnData.y - 1;

	// change original space -> arcgis space
	// to generate arcgis data path
	gis::sDownloadData dnInfo = m_dnData;
	dnInfo.y = y;
	StrPath dstFileName = gis::GetDownloadFileName(g_mediaDir, dnInfo);

	Str256 url;
	switch (m_dnData.layer)
	{
	case eLayerName::DEM:
		cmd = "WorldElevation3D/Terrain3D/ImageServer/tile/";
		url.Format("%s%s%d/%d/%d", site, cmd, level, y, x);
		//dstFileName += ".tmp";
		break;
	case eLayerName::TILE:
		cmd = "World_Imagery/MapServer/tile/";
		url.Format("%s%s%d/%d/%d", site, cmd, level, y, x);
		break;
	default: 
	{
		assert(0); 
		m_geoDownloader->FailDownload(m_dnData);
		m_geoDownloader = nullptr; // finish
		return eRunResult::END;
	}
	break;
	}

	// make directory (level, y directory)
	{
		StrPath dirPath;
		dirPath.Format("%s\\%d", dir, level);
		if (!common::IsFileExist(dirPath))
			_mkdir(dirPath.c_str());

		dirPath.Format("%s\\%d\\%04d", dir, level, y);
		if (!common::IsFileExist(dirPath))
			_mkdir(dirPath.c_str());
	}

	const HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), dstFileName.c_str(), 0, NULL);
	if (S_OK == hr)
	{
		//if (eLayerName::DEM == m_dnData.layer)
		//{
		//	// generate custom dem format file
		//	// to simple heightmap data
		//	// arcgis heightmap data compressed lerc format
		//	gis2::cHeightmap3 hmap;
		//	if (hmap.ReadLerc(dstFileName.c_str()))
		//	{
		//		const StrPath fileName = gis::GetDownloadFileName(g_mediaDir, dnInfo);
		//		hmap.Write(g_mediaDir);
		//		DeleteFileA(dstFileName.c_str()); // remove temporal file
		//	}
		//}
		
		m_geoDownloader->CompleteDownload(m_dnData);
	}
	else
	{
		m_geoDownloader->FailDownload(m_dnData);
	}

	m_geoDownloader = nullptr; // finish
	return eRunResult::END;
}
