
#include "stdafx.h"
#include "geodownloader.h"
#include "../tile/quadtilemanager.h"

using namespace gis;


cGeoDownloader::cGeoDownloader()
	: m_isOfflineMode(false)
	, m_totalDownloadFileSize(0)
	, m_listener(nullptr)
{
	m_tpDownloader.Init(5); // 5 thread
}

cGeoDownloader::~cGeoDownloader()
{
	Clear();
}


// create geo downloader
// setting apikey
bool cGeoDownloader::Create(const string &apiKey
	, iDownloadFinishListener *listener)
{
	m_apiKey = apiKey;
	m_listener = listener;
	return true;
}


bool cGeoDownloader::DownloadFile(const eGeoServer svrType
	, const int level, const int x, const int y
	, const int idx
	, const eLayerName::Enum type
	, const char *dataFileName //= NULL
)
{
	if (m_isOfflineMode)
		return false;

	sDownloadData data;
	ZeroMemory(&data, sizeof(data));
	data.level = level;
	data.x = x;
	data.y = y;
	data.idx = idx;
	data.layer = type;
	data.dataFile = dataFileName;

	const keytype key = MakeKey(data);
	const int taskId = common::GenerateId();
	{
		AutoCSLock cs(m_cs);
		if (m_requestIds.end() != m_requestIds.find(key))
			return false; //already requested
		m_requestIds.insert(key);
		m_taskIds[key] = taskId;
	}

	cTask *task = nullptr;
	switch (svrType)
	{
	case eGeoServer::XDWORLD:
		task = new cTaskWebDownload(taskId, this, data);
		break;
	case eGeoServer::ARCGIS:
		task = new cTaskArcGisDownload(taskId, this, data);
		break;
	case eGeoServer::GOOGLEMAP:
		task = new cTaskGoogleDownload(taskId, this, data);
		break;
	default: assert(0); break;
	}

	if (task)
		m_tpDownloader.PushTask(task);

	return true;
}


// send listener if complete download
void cGeoDownloader::UpdateDownload()
{
	AutoCSLock cs(m_cs);

	for (auto &file : m_complete)
	{
		// calc total download file size
		const char *dir = nullptr;
		switch (file.layer) {
		case eLayerName::DEM: dir = g_mediaDemDir.c_str(); break;
		case eLayerName::TILE: dir = g_mediaTileDir.c_str(); break;
		default: dir = g_mediaDir.c_str(); break;
		}

		const StrPath dstFileName = gis::GetDownloadFileName(dir, file);
		m_totalDownloadFileSize += dstFileName.FileSize();

		if (m_listener)
			m_listener->OnDownloadComplete(file);

		const keytype key = MakeKey(file);
		m_requestIds.erase(key);
		m_taskIds.erase(key);
	}

	m_complete.clear();
}


// cancel download
bool cGeoDownloader::CancelDownload(const int level, const int x, const int y
	, const int idx
	, const eLayerName::Enum type
	, const char *dataFileName //= NULL
)
{
	sDownloadData data;
	ZeroMemory(&data, sizeof(data));
	data.level = level;
	data.x = x;
	data.y = y;
	data.idx = idx;
	data.layer = type;
	data.dataFile = dataFileName;

	{
		AutoCSLock cs(m_cs);
		const keytype key = MakeKey(data);
		auto it = m_taskIds.find(key);
		if (m_taskIds.end() == it)
			return false;
		m_tpDownloader.RemoveTask(it->second);
	}
	return true;
}


// complete download, calling from task downloader
// add complete queue
bool cGeoDownloader::CompleteDownload(const sDownloadData &dnData)
{
	AutoCSLock cs(m_cs);
	m_complete.push_back(dnData);
	return true;
}


// fail download, calling from task downloader
// remove request queue to retry
bool cGeoDownloader::FailDownload(const sDownloadData &dnData)
{
	AutoCSLock cs(m_cs);
	const keytype key = MakeKey(dnData);
	m_requestIds.erase(key);
	m_taskIds.erase(key);
	return true;
}


// make key
// filename hash + (id + layer * 100) + cQuadTree::MakeKey()
cGeoDownloader::keytype cGeoDownloader::MakeKey(const sDownloadData &dnData)
{
	const int key1 = (dnData.dataFile.empty()) ? 0 : dnData.dataFile.GetHashCode();
	const int key2 = ((int)dnData.layer * 100 + dnData.idx);
	const __int64 key3 = cQuadTree<sQuadData>::MakeKey(dnData.level, dnData.x, dnData.y);
	const keytype key = std::make_tuple(key1, key2, key3);
	return key;
}


void cGeoDownloader::Clear()
{
	{
		AutoCSLock cs(m_cs);
		m_complete.clear();
		m_requestIds.clear();
		m_taskIds.clear();
	}

	m_tpDownloader.Clear();
	common::cMemoryPool4<cTaskWebDownload>::Clear();
}
