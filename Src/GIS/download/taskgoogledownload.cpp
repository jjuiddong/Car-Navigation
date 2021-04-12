
#include "stdafx.h"
#include "taskgoogledownload.h"
#include <direct.h> //mkdir
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

using namespace gis;


cTaskGoogleDownload::cTaskGoogleDownload(const int id
	, cGeoDownloader *geoDownloader
	, const sDownloadData &dnData)
	: cTask(id, "cTaskGoogleDownload", true)
	, m_geoDownloader(nullptr)
{
	SetParameter(geoDownloader, dnData);
}

cTaskGoogleDownload::~cTaskGoogleDownload()
{
	Clear();
}


void cTaskGoogleDownload::Clear()
{
	if (m_geoDownloader)
		m_geoDownloader->FailDownload(m_dnData);
}


void cTaskGoogleDownload::SetParameter(cGeoDownloader *geoDownloader
	, const sDownloadData &dnData)
{
	m_geoDownloader = geoDownloader;
	m_dnData = dnData;
}


common::cTask::eRunResult::Enum cTaskGoogleDownload::Run(const double deltaSeconds)
{
	//sample "https://khms1.googleapis.com/kh?v=899&hl=ko-KR&x=1&y=1&z=1";
	const char *site = "https://khms1.googleapis.com/kh?v=899&hl=ko-KR&";
	const char *dir = nullptr;

	// change original space -> googlemap space
	const int level = m_dnData.level;
	const int x = m_dnData.x;
	const int y = (1 << m_dnData.level) - m_dnData.y - 1;

	// change original space -> arcgis space
	// to generate arcgis data path
	gis::sDownloadData dnInfo = m_dnData;
	dnInfo.y = y;
	StrPath dstFileName;

	Str256 url;
	switch (m_dnData.layer)
	{
	case eLayerName::TILE:
		dir = g_mediaTileDir.c_str();
		dstFileName = gis::GetDownloadFileName(g_mediaTileDir, dnInfo);
		url.Format("%sx=%d&y=%d&z=%d", site, x, y, level);
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
		m_geoDownloader->CompleteDownload(m_dnData);
	}
	else
	{
		m_geoDownloader->FailDownload(m_dnData);
	}

	m_geoDownloader = nullptr; // finish
	return eRunResult::END;
}
