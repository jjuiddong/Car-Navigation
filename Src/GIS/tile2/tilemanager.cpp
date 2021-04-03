
#include "stdafx.h"
#include "tilemanager.h"

using namespace graphic;
using namespace gis2;


cTileManager::cTileManager()
{
	m_timer.Create();
	m_geoDownloader.Create("", this);
}

cTileManager::~cTileManager()
{
	Clear();
}


// camLonLat: camera lookat position
bool cTileManager::Update(graphic::cRenderer &renderer
	, cQTerrain &terrain
	, const Vector2d &camLonLat
	, const float deltaSeconds)
{
	static double oldT = m_timer.GetSeconds();
	const double curT = m_timer.GetSeconds();
	m_curFrameT = curT;

	UpdateDownloadFile(renderer);

	//----------------------------------------------------------
	// remove unused tile
	set<__int64> rms;

	const int tileCount = m_tiles.size();
	double limitTime = (tileCount > 1000) ? 0.4f : ((tileCount > 500) ? 0.6f : LIMIT_TIME);
	limitTime = std::max((curT - oldT) * 2.f, limitTime);
	oldT = curT;

	for (auto &kv : m_tiles)
	{
		sTileMem &mem = kv.second;
		cQTile *tile = mem.tile;
		if ((curT - mem.accessTime) > limitTime)
		{
			{
				StrPath fileName = cHeightmap3::GetFileName(
					g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y);
				m_loader1.RemoveParallel<cHeightmap3>(fileName.c_str());
				m_geoDownloader.CancelDownload(
					tile->m_level, tile->m_loc.x, tile->m_loc.y, 0, gis::eLayerName::DEM);
			}

			{
				StrPath fileName = cTileTexture::GetFileName2(
					g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y);
				m_loader1.RemoveParallel<cTileTexture>(fileName.c_str());
				m_geoDownloader.CancelDownload(
					tile->m_level, tile->m_loc.x, tile->m_loc.y, 0, gis::eLayerName::TILE);
			}

			{
				const int64 key = cQuadTree<sQuadData>::MakeKey(
					tile->m_level, tile->m_loc.x, tile->m_loc.y);
				m_heights.erase(key);
			}

			delete mem.tile;
			rms.insert(kv.first);
		}
	}

	for (auto &key : rms)
		m_tiles.erase(key);

	return true;
}


// check complete download file
void cTileManager::UpdateDownloadFile(graphic::cRenderer &renderer)
{
	m_geoDownloader.UpdateDownload();

	for (auto &file : m_downloadFiles)
	{
		switch (file.layer)
		{
		case gis::eLayerName::DEM:
		{
			// complete download heightmap file
			cQTile *tile = FindTile(file.level, file.x, file.y);
			if (tile && !tile->m_hmap)
			{
				StrPath fileName = cHeightmap3::GetFileName(
					g_mediaDir, file.level, file.x, file.y);
				m_loader1.LoadParallel<cHeightmap3>(renderer, fileName.c_str()
					, &tile->m_loadFlag[file.layer], (void**)&tile->m_hmap, true);
			}
		}
		break;

		case gis::eLayerName::TILE:
		{
			// complete download texture file
			cQTile *tile = FindTile(file.level, file.x, file.y);
			if (tile && !tile->m_texture)
			{
				StrPath fileName = cTileTexture::GetFileName2(
					g_mediaDir, file.level, file.x, file.y);
				// check no image
				const int64 fileSize = fileName.FileSize();
				if (fileSize > 3000)
				{
					m_loader1.LoadParallel<cTileTexture>(renderer, fileName.c_str()
						, &tile->m_loadFlag[file.layer], (void**)&tile->m_texture, true);
				}
			}
		}
		break;

		default: assert(0); break;
		}
	}

	m_downloadFiles.clear();
}


// return corespond lv,x,y tile
cQTile* cTileManager::GetTile(graphic::cRenderer &renderer
	, const int level, const int x, const int y
	, const sRectf &rect)
{
	if (!m_tileVtxBuff.m_vtxBuff)
		CreateVertexBuffer(renderer);

	const __int64 key = qtree::MakeKey(level, x, y);
	auto it = m_tiles.find(key);
	if (m_tiles.end() != it)
	{
		it->second.accessTime = m_curFrameT;
		return it->second.tile;
	}

	if (rect.IsEmpty())
		return nullptr;

	cQTile *tile = new cQTile();
	tile->Create(renderer, level, x, y, rect, NULL);
	tile->m_vtxBuff = &m_tileVtxBuff;
	m_tiles[key] = { m_curFrameT, tile };

	return tile;
}


cQTile* cTileManager::FindTile(const int level, const int x, const int y)
{
	const __int64 key = qtree::MakeKey(level, x, y);
	auto it = m_tiles.find(key);
	if (m_tiles.end() != it)
		return it->second.tile;
	return nullptr;
}


// load tile resource
// heightmap, texture
bool cTileManager::LoadResource(graphic::cRenderer &renderer
	, cQTerrain &terrain
	, cQTile &tile
	, const int level, const int x, const int y
	, const sRectf &rect)
{
	if (tile.m_loaded)
	{
		// nothing~
	}
	else
	{
		tile.m_loaded = true;
		tile.m_modelLoadState = 0;
		LoadTexture(renderer, tile, terrain, level, x, y, rect);
		LoadHeightMap(renderer, tile, terrain, level, x, y, rect);
	}

	return true;
}


// load tile texture
void cTileManager::LoadTexture(graphic::cRenderer &renderer
	, cQTile &tile
	, cQTerrain &terrain
	, const int level, const int x, const int y
	, const sRectf &rect)
{
	StrPath fileName = cTileTexture::GetFileName2(g_mediaDir, level, x, y);
	bool isFileExist = fileName.IsFileExist();
	bool isNeedDownload = !isFileExist;
	if (isFileExist)
	{
		// empty file?
		const int64 fileSize = fileName.FileSize();
		if ((tile.m_level < 15) && (fileSize < 3000))
		{
			isFileExist = false;
			isNeedDownload = true;
		}
		else if (fileSize >= 3000)
		{
			m_loader1.LoadParallel<cTileTexture>(renderer, fileName.c_str()
				, &tile.m_loadFlag[gis::eLayerName::TILE], (void**)&tile.m_texture, true);
		}
	}

	if (isNeedDownload)
	{
		m_geoDownloader.DownloadFile(gis::eGeoServer::ARCGIS
			, level, x, y, 0, gis::eLayerName::TILE);
	}

	// replace parent texture until download or load texture
	ReplaceParentTexture(renderer, terrain, tile, level, x, y, rect, 0, isFileExist);
}


// load heightmap
bool cTileManager::LoadHeightMap(graphic::cRenderer &renderer
	, cQTile &tile
	, cQTerrain &terrain
	, const int level, const int x, const int y
	, const sRectf &rect)
{
	if (tile.m_hmap)
		return true;

	const int ty = (1 << level) - y - 1;
	StrPath fileName = cHeightmap3::GetFileName(g_mediaDir, level, x, y);
	bool isFileExist = fileName.IsFileExist();
	bool isNeedDownload = !isFileExist;
	if (isFileExist)
	{
		// empty file?
		const int64 fileSize = fileName.FileSize();
		if ((tile.m_level < 15) && (fileSize < 200))
		{
			// null heightmap, no load this file
			isFileExist = false;
			isNeedDownload = true;
		}
		else if (fileSize >= 200)
		{
			m_loader1.LoadParallel<cHeightmap3>(renderer, fileName.c_str()
				, &tile.m_loadFlag[gis::eLayerName::DEM], (void**)&tile.m_hmap, true);
		}
	}

	if (isNeedDownload)
	{
		m_geoDownloader.DownloadFile(gis::eGeoServer::ARCGIS
			, level, x, y, 0, gis::eLayerName::DEM);
	}

	// replace parent heightmap until download or load heightmap
	ReplaceParentTexture(renderer, terrain, tile, level, x, y, rect, 1, isFileExist);
	return true;
}


void cTileManager::CreateVertexBuffer(graphic::cRenderer &renderer)
{
	if (!m_tileVtxBuff.m_vtxBuff)
	{
		sVertexNormTex vtx[4] = {
			{ Vector3(0,0,1), Vector3(0,1,0), 0, 0 }
			,{ Vector3(1,0,1), Vector3(0,1,0), 1, 0 }
			,{ Vector3(0,0,0), Vector3(0,1,0), 0, 1 }
			,{ Vector3(1,0,0), Vector3(0,1,0), 1, 1 }
		};
		m_tileVtxBuff.Create(renderer, 4, sizeof(sVertexNormTex), vtx);
	}

	if (!m_tileLineVtxBuff.m_vtxBuff)
	{
		sVertexNormTex vtx[4] = {
			{ Vector3(0,0,1), Vector3(0,1,0), 0, 0 }
			,{ Vector3(1,0,1), Vector3(0,1,0), 1, 0 }
			,{ Vector3(0,0,0), Vector3(0,1,0), 0, 1 }
			,{ Vector3(1,0,0), Vector3(0,1,0), 1, 1 }
		};
		m_tileLineVtxBuff.Create(renderer, 4, sizeof(sVertexNormTex), vtx);
	}
}


// replace texture until load complete
// texType: 0=texture, 1=heightmap
bool cTileManager::ReplaceParentTexture(graphic::cRenderer &renderer
	, cQTerrain &terrain
	, cQTile &tile
	, const int level, const int x, const int y
	, const sRectf &rect
	, const int texType
	, const bool existFile
)
{
	cQTile *parentTile = GetReplaceNode(terrain, level, x, y, texType);

	// modify uv position
	if (!parentTile)
		return false;

	// rect space and 3d space is diffrent z axis direction, need flip
	// solution is convert negativ to positive value
	const sRectf &parentR = parentTile->m_rect;
	const Vector2 lt = Vector2(parentR.left, parentR.bottom); // left-top
	const Vector2 rb = Vector2(parentR.right, parentR.top); // right-bottom
	const Vector2 clt = Vector2(rect.left, rect.bottom); // current rect left-top
	const Vector2 crb = Vector2(rect.right, rect.top); // current rect right-bottom

	const float w = parentR.Width();
	const float h = parentR.Height();
	const float u1 = abs((clt.x - lt.x) / w);
	const float v1 = abs((clt.y - lt.y) / h);
	const float u2 = abs((crb.x - lt.x) / w);
	const float v2 = abs((crb.y - lt.y) / h);

	if (0 == texType) // texture
	{
		tile.m_tuvs[0] = u1;
		tile.m_tuvs[1] = v1;
		tile.m_tuvs[2] = u2;
		tile.m_tuvs[3] = v2;

		tile.m_replaceTex = parentTile->m_texture;
	}
	else if (1 == texType) // heightmap
	{
		tile.m_huvs[0] = u1;
		tile.m_huvs[1] = v1;
		tile.m_huvs[2] = u2;
		tile.m_huvs[3] = v2;
		tile.m_replaceHmap = nullptr;

		cHeightmap3 *replaceHmap = parentTile->m_hmap;			
		if (!tile.m_hmap && replaceHmap)
		{
			tile.m_replaceHmap = replaceHmap;
		}
	}

	return true;
}


// return replace node
// texType : 0=texture, 1=heightmap
cQTile* cTileManager::GetReplaceNode(cQTerrain &terrain
	, const int level, const int x, const int y
	, const int texType
)
{
	int lv = level;
	int px = x;
	int py = y;
	cQTile *parentTile = NULL;

	// find parent node
	do
	{
		--lv;
		px >>= 1;
		py >>= 1;
		cQTile *tile = FindTile(lv, px, py);

		if (0 == texType) // texture
		{
			if (tile && tile->m_texture)
				parentTile = tile;
		}
		else if (1 == texType) // heightmap
		{
			if (tile && tile->m_hmap)
				parentTile = tile;
		}
		else
		{
			assert(0);
		}

	} while ((lv > 0) && !parentTile);

	return parentTile;
}


// web downloader, download complete event handler
void cTileManager::OnDownloadComplete(const gis::sDownloadData &data)
{
	m_downloadFiles.push_back(data);
}


void cTileManager::Clear()
{
	//m_readyLoadModel.clear();
	m_geoDownloader.Clear();

	m_loader1.ClearFile<cTileTexture>();
	m_loader1.ClearFile<cHeightmap3>();

	m_geoDownloader.Clear();
	m_heights.clear();

	for (auto &kv : m_tiles)
		delete kv.second.tile;
	m_tiles.clear();
}
