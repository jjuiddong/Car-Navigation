
#include "stdafx.h"
#include "quadtilemanager.h"

using namespace graphic;


cQuadTileManager::cQuadTileManager()
	: m_timeSortingLoadModel(0)
	, m_timeLoadFileSorting(0)
	, m_cntRemoveTile(0)
	, m_isDeepCopySmooth(true)
{
	m_timer.Create();
}

cQuadTileManager::~cQuadTileManager()
{
	Clear();
}


bool cQuadTileManager::Update(graphic::cRenderer &renderer, cTerrainQuadTree &terrain
	, const Vector2d &camLonLat, float deltaSeconds)
{
	static double oldT = m_timer.GetSeconds();
	const double curT = m_timer.GetSeconds();
	m_curFrameT = curT;

	SortingDownloadFile(camLonLat, terrain, deltaSeconds);
	UpdateDownloadFile(renderer);
	UpdateLoadModelFile(renderer, camLonLat, terrain, deltaSeconds);

	//----------------------------------------------------------
	// remove unused tile
	set<__int64> rms;

	const int tileCount = m_tiles.size();
	double limitTime = (tileCount > 1000) ? 0.4f : ((tileCount > 500) ? 0.6f : LIMIT_TIME);
	//if (limitTime < (curT - oldT) + 20)
	limitTime = max((curT - oldT) * 2.f, limitTime); // FPS 가 낮을 경우를 대비
	oldT = curT;

	m_cntRemoveTile = 0;

	for (auto &kv : m_tiles)
	{
		sTileMem &mem = kv.second;
		cQuadTile *tile = mem.tile;
		if ((curT - mem.accessTime) > limitTime)
		{
			++m_cntRemoveTile;

			{
				StrPath fileName = cHeightmap2::GetFileName(g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y);
				m_loader1.RemoveParallel<cHeightmap2>(fileName.c_str());
			}

			{
				// remove replace deepcopy heightmap
				StrPath fileName = cHeightmap2::GetFileName(g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y) + "DeepCpy";
				m_loader1.RemoveParallel<cHeightmap2>(fileName.c_str());
			}

			{
				StrPath fileName = cTileTexture::GetFileName(g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y);
				m_loader1.RemoveParallel<cTileTexture>(fileName.c_str());
			}

			{
				StrPath fileName = cPoiReader::GetFileName(g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y, gis::eLayerName::POI_BASE);
				m_loader2.RemoveParallel<cPoiReader>(fileName.c_str());
			}

			{
				StrPath fileName = cPoiReader::GetFileName(g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y, gis::eLayerName::POI_BOUND);
				m_loader2.RemoveParallel<cPoiReader>(fileName.c_str());
			}

			{
				const int64 key = cQuadTree<sQuadData>::MakeKey(tile->m_level, tile->m_loc.x, tile->m_loc.y);
				m_heights.erase(key);
			}

			if (tile->m_facilityIndex)
			{
				for (u_int i = 0; i < tile->m_facilityIndex->m_objs.size(); ++i)
				{
					if (tile->m_facilities.size() > i)
					{
						auto &facility = tile->m_facilities[i];
						if (facility && !facility->m_xdos.empty())
						{
							StrPath fileName = cTileTexture::GetFileName(g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y
								, facility->m_xdos[0].imageName);
							m_loader1.RemoveParallel<cTileTexture>(fileName.c_str());
						}
					}

					auto &obj = tile->m_facilityIndex->m_objs[i];
					{
						StrPath fileName = cXdoReader::GetFileName(g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y
							, obj.dataFile);
						m_loader2.RemoveParallel<cXdoReader>(fileName.c_str());
					}
				}
			}

			{
				StrPath fileName = cReal3DModelIndexReader::GetFileName(g_mediaDir, tile->m_level, tile->m_loc.x, tile->m_loc.y);
				m_loader2.RemoveParallel<cReal3DModelIndexReader>(fileName.c_str());
			}

			delete mem.tile;
			rms.insert(kv.first);
		}
	}

	for (auto &key : rms)
		m_tiles.erase(key);

	//// time check
	//const double afterT = m_timer.GetSeconds();
	//static double maxT = 0;
	//if (afterT - curT > maxT)
	//{
	//	maxT = afterT - curT;
	//	dbg::Logp("MaxT = %f \n", maxT);
	//}

	return true;
}


void cQuadTileManager::SortingDownloadFile(const Vector2d &camLonLat, cTerrainQuadTree &terrain
	, const float deltaSeconds)
{
	// 카메라가 바라보는 방향의 파일을 먼저 읽어들인다.
	//class cSortTask : public iSortingTasks
	//{
	//public:
	//	cSortTask(const Vector2d &lonLat) : m_lonLat(lonLat) {}
	//	Vector2d m_lonLat;

	//	virtual void Sorting(vector<cTask*> &tasks) override
	//	{
	//		struct CompareFacility
	//		{
	//			CompareFacility(const Vector2d &lonLat) : mLonLat((float)lonLat.x, 0, (float)lonLat.y) {}
	//			Vector3 mLonLat;

	//			bool operator () (cTask *t1, cTask *t2)
	//			{
	//				typedef cTaskFileLoader<cXdoReader, 5000, sFileLoaderArg, sDistanceCompare> TaskType;
	//				TaskType *a1 = (TaskType*)t1;
	//				TaskType *a2 = (TaskType*)t2;
	//				
	//				const Vector3 p1 = { (float)a1->m_compare.lonLat.x, 0, (float)a1->m_compare.lonLat.y };
	//				const Vector3 p2 = { (float)a2->m_compare.lonLat.x, 0, (float)a2->m_compare.lonLat.y };
	//				const float d1 = p1.Distance(mLonLat);
	//				const float d2 = p2.Distance(mLonLat);
	//				return d1 > d2;
	//			}
	//		};

	//		std::sort(tasks.begin(), tasks.end(), CompareFacility(m_lonLat));
	//	}
	//};


	// 1초마다 한번씩 소팅
	m_timeLoadFileSorting += deltaSeconds;
	if (m_timeLoadFileSorting > 1.f)
	{
		m_timeLoadFileSorting = 0;
		//m_modelIndices.m_thread.SortTasks(new cSortTask(camLonLat));
		//m_facilities.m_thread.SortTasks(new cSortTask(camLonLat));
		//m_facilitiesTex.m_thread.SortTasks(new cSortTask(camLonLat));
	}
}


// 매 프레임마다, 다운로드완료된 파일을 검사해서, 업데이트 한다.
void cQuadTileManager::UpdateDownloadFile(graphic::cRenderer &renderer)
{
	m_geoDownloader.UpdateDownload();

	for (auto &file : m_downloadFiles)
	{
		switch (file.layer)
		{
		case gis::eLayerName::DEM:
		{
			// complete download *.bil file
			cQuadTile *tile = FindTile(file.level, file.xLoc, file.yLoc);
			if (tile && !tile->m_hmap)
			{
				StrPath fileName = cHeightmap2::GetFileName(g_mediaDir, file.level, file.xLoc, file.yLoc);
				m_loader1.LoadParallel<cHeightmap2>(renderer, fileName.c_str()
					, &tile->m_loadFlag[file.layer], (void**)&tile->m_hmap, true);
			}
		}
		break;

		case gis::eLayerName::TILE:
		{
			// complete download *.dds file
			cQuadTile *tile = FindTile(file.level, file.xLoc, file.yLoc);
			if (tile && !tile->m_texture)
			{
				StrPath fileName = cTileTexture::GetFileName(g_mediaDir, file.level, file.xLoc, file.yLoc);
				m_loader1.LoadParallel<cTileTexture>(renderer, fileName.c_str()
					, &tile->m_loadFlag[file.layer], (void**)&tile->m_texture, true);
			}
		}
		break;

		case gis::eLayerName::POI_BASE:
		{
			// complete download *.poi_base file
			cQuadTile *tile = FindTile(file.level, file.xLoc, file.yLoc);
			if (tile && !tile->m_poi[0])
			{
				StrPath fileName = cPoiReader::GetFileName(g_mediaDir, file.level, file.xLoc, file.yLoc, gis::eLayerName::POI_BASE);
				m_loader2.LoadParallel<cPoiReader>(renderer, fileName.c_str()
					, &tile->m_loadFlag[file.layer], (void**)&tile->m_poi[0], true);
			}
		}
		break;

		case gis::eLayerName::POI_BOUND:
		{
			// complete download *.poi_bound file
			cQuadTile *tile = FindTile(file.level, file.xLoc, file.yLoc);
			if (tile && !tile->m_poi[1])
			{
				StrPath fileName = cPoiReader::GetFileName(g_mediaDir, file.level, file.xLoc, file.yLoc, gis::eLayerName::POI_BOUND);
				m_loader2.LoadParallel<cPoiReader>(renderer, fileName.c_str()
					, &tile->m_loadFlag[file.layer], (void**)&tile->m_poi[1], true);
			}
		}
		break;

		case gis::eLayerName::FACILITY_BUILD:
		{
			// complete download *.facility_build file
			cQuadTile *tile = FindTile(file.level, file.xLoc, file.yLoc);
			if (tile && !tile->m_facilityIndex)
			{
				tile->m_modelLoadState = 2;

				//sDistanceCompare compData;
				//compData.lonLat = gis::TileLoc2WGS84(tile->m_level, tile->m_loc.x, tile->m_loc.y);
				StrPath fileName = cReal3DModelIndexReader::GetFileName(g_mediaDir, file.level, file.xLoc, file.yLoc);
				m_loader2.LoadParallel<cReal3DModelIndexReader>(renderer, fileName.c_str()
					, &tile->m_loadFlag[file.layer], (void**)&tile->m_facilityIndex, false);
			}
			else
			{
				if (tile)
					tile->m_modelLoadState = 0;
			}
		}
		break;

		case gis::eLayerName::FACILITY_BUILD_GET:
		{
			// complete download *.xdo file
			cQuadTile *tile = FindTile(file.level, file.xLoc, file.yLoc);
			if (tile
				&& tile->m_facilityIndex
				&& (tile->m_facilityIndex->m_objs.size() > (u_int)file.idx)
				&& (tile->m_facilities.size() > (u_int)file.idx)
				&& !tile->m_facilities[file.idx]
				)
			{
				//sDistanceCompare compData;
				//compData.lonLat = tile->m_facilityIndex->m_objs[file.idx].centerPos;
				StrPath fileName = cXdoReader::GetFileName(g_mediaDir, file.level, file.xLoc, file.yLoc
					, file.dataFile.c_str());
				m_loader2.LoadParallel<cXdoReader>(renderer, fileName.c_str(), &tile->m_loadFlag[file.layer]
					, (void**)&tile->m_facilities[file.idx], false);
			}
			else
			{
				if (tile && (tile->m_facilities.size() > (u_int)file.idx))
					tile->m_facilities[file.idx]->m_loadState = cXdoReader::FINISH;
			}
		}
		break;

		case gis::eLayerName::FACILITY_BUILD_GET_JPG:
		{
			// complete download *.jpg file
			cQuadTile *tile = FindTile(file.level, file.xLoc, file.yLoc);
			if (tile 
				&& (tile->m_facilitiesTex.size() > (u_int)file.idx) 
				&& !tile->m_facilitiesTex[file.idx])
			{
				StrPath fileName = cTileTexture::GetFileName(g_mediaDir, file.level, file.xLoc, file.yLoc
					, file.dataFile.c_str());
				m_loader1.LoadParallel<cTileTexture>(renderer, fileName.c_str(), &tile->m_loadFlag[file.layer]
					, (void**)&tile->m_facilitiesTex[file.idx], true);
			}
			else
			{
				if (tile && (tile->m_facilities.size() > (u_int)file.idx))
					tile->m_facilities[file.idx]->m_loadState = cXdoReader::FINISH;
			}
		}
		break;

		default: assert(0); break;
		}
	}

	m_downloadFiles.clear();
}


void cQuadTileManager::UpdateLoadModelFile(graphic::cRenderer &renderer
	, const Vector2d &camLonLat, cTerrainQuadTree &terrain
	, const float deltaSeconds
)
{
	RET(m_readyLoadModel.empty());

	// 정렬, 카메라와 가까운 모델을 먼저 로딩한다.
	struct sModelCompare
	{
		sModelCompare(const Vector2d &lonLat) : mLonLat((float)lonLat.x, 0, (float)lonLat.y) {}
		Vector3 mLonLat;// 카메라가 가르키는 경위도
		bool operator() (const sReadyLoadModel &a1, const sReadyLoadModel &a2)
		{
			const Vector3 p1 = { (float)a1.lonLat.x, 0, (float)a1.lonLat.y };
			const Vector3 p2 = { (float)a2.lonLat.x, 0, (float)a2.lonLat.y };
			const float d1 = p1.Distance(mLonLat);
			const float d2 = p2.Distance(mLonLat);
			return d1 > d2;
		}
	};

	// 1초 한 번씩 소팅
	m_timeSortingLoadModel += deltaSeconds;
	if (m_timeSortingLoadModel > 1.f)
	{
		m_timeSortingLoadModel = 0;
		std::sort(m_readyLoadModel.begin(), m_readyLoadModel.end(), sModelCompare(camLonLat));
	}

	m_timeLoadModel += deltaSeconds;
	if (m_timeLoadModel > 1.f)
	{
		//common::dbg::Logp("cntLoadModelOneSecond = %d, timeLoadBalancing=%f\n"
		//	, m_cntLoadModelOneSecond, m_timeLoadBalancing);

		m_timeLoadModel = 0;
		//m_timeLoadBalancing = 0;
		m_cntLoadModelOneSecond = 0;
	}

	m_timeLoadBalancing = 0;
	int cnt = 0; // 1frame maximum load model count = 10
	while (!m_readyLoadModel.empty() 
		&& (m_timeLoadBalancing < 0.02f) 
		//&& (cnt < 30)
		)
	{
		const double t0 = m_timer.GetSeconds();

		const sReadyLoadModel &modInfo = m_readyLoadModel.back();
		if (CreateFacility(renderer, terrain, modInfo))
		{
			++cnt;
			++m_cntLoadModelOneSecond;
		}
		m_readyLoadModel.pop_back();

		const double t1 = m_timer.GetSeconds();
		m_timeLoadBalancing += (t1 - t0);
	}
}


bool cQuadTileManager::CreateFacility(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain
	, const sReadyLoadModel &modInfo)
{
	cQuadTile *tile = FindTile(modInfo.level, modInfo.loc.x, modInfo.loc.y);
	if (!tile)
		return false;

	const u_int i = modInfo.facilityIdx;
	if (tile->m_facilities.size() <= i)
		return false;

	cXdoReader *xdo = tile->m_facilities[i];
	if (!xdo)
		return false;

	// 건물 3d파일 로딩이 완료되면, 3D 모델로 변환한다.
	// 이미 로딩되었는지 확인한다.
	auto result = graphic::cResourceManager::Get()->FindAssimpModel(xdo->m_fileName.c_str());
	if (!result.first) // not exist assimp model
	{
		// 건물마다 위치와 모양이 유니크하기 때문에 가능한 코드
		xdo->m_lonLat = tile->m_facilityIndex->m_objs[i].centerPos;
		xdo->LoadMesh(renderer);
		tile->m_mods[i].Create(renderer, i, xdo->m_fileName.c_str());
	}
	else
	{
		tile->m_mods[i].Create(renderer, i, xdo->m_fileName.c_str());
	}

	xdo->CalcAltitude(tile->m_facilityIndex->m_objs[i].centerPos);

	tile->m_mods[i].SetRenderFlag(eRenderFlag::ALPHABLEND, false);
	tile->m_mods[i].SetRenderFlag(eRenderFlag::NOALPHABLEND, true);

	// set facility position
	Vector2d lonLat = tile->m_facilityIndex->m_objs[i].centerPos;
	const Vector3 globalPos = gis::WGS842Pos(lonLat);
	const Vector3 relPos = gis::GetRelationPos(globalPos);
	tile->m_mods[i].m_transform.pos = relPos;
	tile->m_mods[i].m_transform.pos.y = xdo->m_altitude2 * 0.00001f * 2500.f;

	const Vector3 pos3D = terrain.Get3DPos(lonLat);
	tile->m_mods[i].m_transform.pos.y -= (tile->m_mods[i].m_transform.pos.y - pos3D.y) * 0.3f;
	tile->m_mods[i].m_transform.scale = Vector3(1, 1, 1)*g_root.m_scale;

	Quaternion q;
	q.SetRotationY(ANGLE2RAD(g_root.m_angleY));
	tile->m_mods[i].m_transform.rot = q;

	return true;
}


cQuadTile* cQuadTileManager::GetTile(graphic::cRenderer &renderer
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect)
{
	if (!m_tileVtxBuff.m_vtxBuff)
		CreateVertexBuffer(renderer);

	const __int64 key = cQuadTree<sQuadData>::MakeKey(level, xLoc, yLoc);
	auto it = m_tiles.find(key);
	if (m_tiles.end() != it)
	{
		it->second.accessTime = m_curFrameT;
		return it->second.tile;
	}

	if (rect.IsEmpty())
		return NULL;

	cQuadTile *tile = new cQuadTile();
	tile->Create(renderer, level, xLoc, yLoc, rect, NULL);
	tile->m_vtxBuff = &m_tileVtxBuff;
	m_tiles[key] = { m_curFrameT, tile };

	return tile;
}


cQuadTile* cQuadTileManager::FindTile(const int level, const int xLoc, const int yLoc)
{
	const __int64 key = cQuadTree<sQuadData>::MakeKey(level, xLoc, yLoc);
	auto it = m_tiles.find(key);
	if (m_tiles.end() != it)
		return it->second.tile;
	return NULL;
}


// 해당 타일의 가장 높은 높이값을 리턴한다.
// tile에 저장되어 있음.
float cQuadTileManager::GetMaximumHeight(const int level, const int xLoc, const int yLoc)
{
	const __int64 key = cQuadTree<sQuadData>::MakeKey(level, xLoc, yLoc);
	auto it = m_heights.find(key);
	if (m_heights.end() != it)
		return it->second;

	// 부모 타일을 검사해서 더 큰쪽으로 저장한다.
	RETV(level <= 1, cHeightmap2::DEFAULT_H);

	const cQuadTile *tile = FindTile(level, xLoc, yLoc);
	float height = cHeightmap2::DEFAULT_H;
	if (level > 1)
	{
		// find parent tile heightmap
		int plevel = level - 1;
		int pxLoc = xLoc >> 1;
		int pyLoc = yLoc >> 1;
		cHeightmap2 *ph = NULL;
		cQuadTile *ptile = FindTile(plevel, pxLoc, pyLoc);
		while (ptile && (plevel > 1))
		{
			if (ptile->m_hmap)
			{
				ph = ptile->m_hmap;
				break;
			}
			else
			{
				--plevel;
				pxLoc >>= 1;
				pyLoc >>= 1;
				ptile = FindTile(plevel, pxLoc, pyLoc);
			}
		}

		cHeightmap2 *h = (tile && tile->m_hmap) ? tile->m_hmap : NULL;

		if (ph && h)
		{
			height = max(ph->m_maxHeight, h->m_maxHeight);
			m_heights.insert({ key, height });
		}
		else if (!ph && h)
		{
			height = h->m_maxHeight;
		}
		else if (ph && !h)
		{
			height = ph->m_maxHeight;
		}
	}
	else
	{
		cHeightmap2 *h = (tile && tile->m_hmap) ? tile->m_hmap : NULL;
		if (h)
		{
			height = h->m_maxHeight;
			m_heights.insert({ key, height });
		}
	}

	return height;
}


void cQuadTileManager::CreateVertexBuffer(graphic::cRenderer &renderer)
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


// 리소스 로딩
// - 텍스쳐
// - 높이맵
bool cQuadTileManager::LoadResource(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain
	, cQuadTile &tile
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect)
{
	if (tile.m_loaded)
	{
		// *.facility_build 파일 로딩 완료?
		if ((2 == tile.m_modelLoadState) && tile.m_facilityIndex)
		{
			// *.xdo 건물 파일을 로딩한다.
			LoadFacilities(renderer, tile, terrain, level, xLoc, yLoc, rect);
		}
		else if (3 == tile.m_modelLoadState)
		{
			int readyCnt = 0;
			for (u_int i = 0; i < tile.m_facilities.size(); ++i)
			{
				cXdoReader *xdo = tile.m_facilities[i];
				if (!xdo)
				{
					++readyCnt;
					continue;
				}
				if (cXdoReader::FINISH == xdo->m_loadState)
					continue;

				++readyCnt;
				switch (xdo->m_loadState)
				{
				case cXdoReader::READ_MODEL:
				{
					xdo->m_loadState = cXdoReader::READ_TEXTURE;

					// Loading Texture
					const StrPath fileName = cTileTexture::GetFileName(g_mediaDir, level, xLoc, yLoc
						, xdo->m_xdos[0].imageName);
					if (fileName.IsFileExist())
					{
						m_loader1.LoadParallel<cTileTexture>(renderer, fileName.c_str()
							, &tile.m_loadFlag[gis::eLayerName::FACILITY_BUILD_GET_JPG]
							, (void**)&tile.m_facilitiesTex[i], true);
					}
					else
					{
						m_geoDownloader.DownloadFile(level, xLoc, yLoc, i
							, gis::eLayerName::FACILITY_BUILD_GET_JPG, *this, this
							, xdo->m_xdos[0].imageName);
					}
				}
				break;

				case cXdoReader::READ_TEXTURE:
				{
					// Check Model Load
					//const StrPath fileName = cTileTexture::GetFileName(g_mediaDir, level, xLoc, yLoc
					//	, xdo->m_xdos[0].imageName);
					//auto result = m_facilitiesTex.Find(fileName.c_str());
					//if (std::get<0>(result))
					//{
					//	// If Not Exist, Re Load
					//	m_facilitiesTex.LoadParallel(renderer, fileName.c_str()
					//		, &tile.m_loadFlag[gis::eLayerName::FACILITY_BUILD_GET_JPG]
					//		, (void**)&tile.m_facilitiesTex[i], true);
					//} 
					//else 
						
					if (tile.m_facilitiesTex[i])
					{
						xdo->m_loadState = cXdoReader::FINISH;

						sReadyLoadModel readyMod;
						readyMod.level = tile.m_level;
						readyMod.loc = tile.m_loc;
						readyMod.facilityIdx = i;
						readyMod.lonLat = tile.m_facilityIndex->m_objs[i].centerPos;
						m_readyLoadModel.push_back(readyMod);
					}
				}
				break;

				default: assert(0); break;
				}
			}

			if (readyCnt == 0)
				tile.m_modelLoadState = 4; // finish facilities load
		}
	}
	else
	{
		tile.m_loaded = true;
		tile.m_modelLoadState = 0;
		LoadTexture(renderer, tile, terrain, level, xLoc, yLoc, rect);
		LoadHeightMap(renderer, tile, terrain, level, xLoc, yLoc, rect);
		LoadPoiFile(renderer, tile, terrain, level, xLoc, yLoc, rect);

		if (terrain.m_isShowFacility)
			LoadFacilityIndex(renderer, tile, terrain, level, xLoc, yLoc, rect);
	}

	return true;
}


// 3D 모델파일을 로딩한다.
bool cQuadTileManager::LoadFacilities(graphic::cRenderer &renderer
	, cQuadTile &tile
	, cTerrainQuadTree &terrain
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect)
{
	RETV2(!tile.m_facilityIndex, false);

	if (tile.m_facilityIndex->m_objs.empty())
	{
		tile.m_modelLoadState = 0;
		return true;
	}

	tile.m_modelLoadState = 3;
	tile.m_facilities.resize(tile.m_facilityIndex->m_objs.size(), NULL);
	tile.m_facilitiesTex.resize(tile.m_facilityIndex->m_objs.size(), NULL);
	tile.m_mods.resize(tile.m_facilityIndex->m_objs.size());

	for (u_int i=0; i < tile.m_facilityIndex->m_objs.size(); ++i)
	{
		auto &obj = tile.m_facilityIndex->m_objs[i];

		// 모델 파일을 로딩한다. 없다면 다운로드 받는다.
		{
			const StrPath fileName = cXdoReader::GetFileName(g_mediaDir, level, xLoc, yLoc, obj.dataFile);
			if (fileName.IsFileExist())
			{
				//sDistanceCompare compData;
				//compData.lonLat = obj.centerPos;
				m_loader2.LoadParallel<cXdoReader>(renderer, fileName.c_str()
					, &tile.m_loadFlag[gis::eLayerName::FACILITY_BUILD_GET]
					, (void**)&tile.m_facilities[i]);

				if (tile.m_facilities[i])
				{
					tile.m_facilities[i]->m_loadState = cXdoReader::READ_MODEL;
				}
			}
			else
			{
				m_geoDownloader.DownloadFile(level, xLoc, yLoc, i
					, gis::eLayerName::FACILITY_BUILD_GET, *this, this
					, obj.dataFile);
			}
		}
	}

	return true;
}


// tile을 중심으로 west, south 위치의 타일의 높이와 부드럽게 맞춰준다.
// 다른 타일과 높이를 맞추기 위해 heightmap에 outer memory가 존재한다.
//
// west (left)
//  :::::::::::::::::::::::
//  ::                   ::
//  ::                   ::
//  ::                   ::
//  ::  inner heightmap  ::
//  ::     memory        ::
//  ::                   ::
//  ::                   ::
//  ::::::::::::::::::::::: outer memory
//	  south (bottom)
//
bool cQuadTileManager::Smooth(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain
	, sQuadTreeNode<sQuadData> *node)
{
	RETV(!node->data.tile->m_hmap, false);

	int isUpdateTex = 0;

	if (sQuadTreeNode<sQuadData> *west = terrain.m_qtree.GetWestNeighbor(node))
	{
		// 한개의 노드가 여러개의 노드들과 붙어있을 경우, 모두 처리해줘야 한다. (서쪽)
		int startY, endY;
		if (west->level > node->level)
		{
			startY = west->yLoc;
			endY = west->yLoc + (1 << abs(node->level - node->data.level[eDirection::WEST]));
		}
		else
		{
			startY = west->yLoc;
			endY = west->yLoc + 1;
		}

		for (int y = startY; y < endY; ++y)
			if (cQuadTile *tile = GetTile(renderer, west->level, west->xLoc, y, sRectf()))
				isUpdateTex += node->data.tile->Smooth(*tile, eDirection::WEST);
	}

	if (sQuadTreeNode<sQuadData> *south = terrain.m_qtree.GetSouthNeighbor(node))
	{
		// 한개의 노드가 여러개의 노드들과 붙어있을 경우, 모두 처리해줘야 한다. (남쪽)
		int startX, endX;
		if (south->level > node->level)
		{
			startX = south->xLoc;
			endX = south->xLoc + (1 << abs(node->level - node->data.level[eDirection::SOUTH]));
		}
		else
		{
			startX = south->xLoc;
			endX = south->xLoc + 1;
		}

		for (int x = startX; x<endX; ++x)
			if (cQuadTile *tile = GetTile(renderer, south->level, x, south->yLoc, sRectf()))
				isUpdateTex += node->data.tile->Smooth(*tile, eDirection::SOUTH);

		// south-west
		//if (sQuadTreeNode<sQuadData> *sw = terrain.m_qtree.GetWestNeighbor(south))
		//	if (cQuadTile *tile = GetTile(renderer, terrain, sw->level, sw->xLoc, sw->yLoc, sRectf()))
		//		node->data.tile->SmoothPostProcess(*tile);
	}

	if (isUpdateTex > 0)
		node->data.tile->UpdateTexture(renderer);

	return true;
}


// 텍스쳐 로딩
void cQuadTileManager::LoadTexture(graphic::cRenderer &renderer
	, cQuadTile &tile
	, cTerrainQuadTree &terrain
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect)
{
	// 파일이 있다면, 텍스쳐를 로딩한다.
	StrPath fileName = cTileTexture::GetFileName(g_mediaDir, level, xLoc, yLoc);
	const bool isFileExist = fileName.IsFileExist();
	if (isFileExist)
	{
		m_loader1.LoadParallel<cTileTexture>(renderer, fileName.c_str()
			, &tile.m_loadFlag[gis::eLayerName::TILE], (void**)&tile.m_texture, true);
	}
	else
	{
		m_geoDownloader.DownloadFile(level, xLoc, yLoc, 0, gis::eLayerName::TILE, *this, this);
	}

	// 텍스쳐 파일이 없다면, 더 높은 레벨의 텍스쳐를 이용하고, uv를 재조정한다.
	// 텍스쳐 로딩이 병렬로 이뤄지기 때문에, 로딩이 이뤄지는 동안, 대체 텍스쳐를 이용한다.
	ReplaceParentTexture(renderer, terrain, tile, level, xLoc, yLoc, rect, 0, isFileExist);
}


// 높이맵 파일 로딩
bool cQuadTileManager::LoadHeightMap(graphic::cRenderer &renderer
	, cQuadTile &tile
	, cTerrainQuadTree &terrain
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect)
{
	if (tile.m_hmap)
		return true;

	StrPath fileName = cHeightmap2::GetFileName(g_mediaDir, level, xLoc, yLoc);
	bool isFileExist = false;
	if (fileName.IsFileExist())
	{
		//if ((tile.m_level == 15) && (fileName.FileSize() < 150))
		const int64 fileSize = fileName.FileSize();
		if ((tile.m_level >= 14) && (fileSize < 150))
		{
			// null heightmap, no load this file
		}
		else if (fileSize > 150)
		{
			isFileExist = true;
			m_loader1.LoadParallel<cHeightmap2>(renderer, fileName.c_str()
				, &tile.m_loadFlag[gis::eLayerName::DEM], (void**)&tile.m_hmap, true);
		}
	}
	else
	{
		m_geoDownloader.DownloadFile(level, xLoc, yLoc, 0, gis::eLayerName::DEM, *this, this);
	}

	// 텍스쳐 파일이 없다면, 더 높은 레벨의 텍스쳐를 이용하고, uv를 재조정한다.
	// 텍스쳐 로딩이 병렬로 이뤄지기 때문에, 로딩이 이뤄지는 동안, 대체 텍스쳐를 이용한다.
	ReplaceParentTexture(renderer, terrain, tile, level, xLoc, yLoc, rect, 1, isFileExist);

	return true;
}


// 높이맵 로딩, 싱글쓰레딩
bool cQuadTileManager::LoadHeightMapDirect(graphic::cRenderer &renderer
	, cQuadTile &tile
	, cTerrainQuadTree &terrain
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect)
{
	if (tile.m_hmap)
		return true;

	const StrPath fileName = cHeightmap2::GetFileName(g_mediaDir, level, xLoc, yLoc);
	if (!fileName.IsFileExist() || (fileName.FileSize() < 150))
		return false;

	tile.m_hmap = new cHeightmap2();
	if (!tile.m_hmap->Read(fileName.c_str()))
	{
		SAFE_DELETE(tile.m_hmap);
		return false;
	}

	return true;
}


// 지역정보, 행정경계 파일 요청 poi_base, poi_bound
bool cQuadTileManager::LoadPoiFile(graphic::cRenderer &renderer
	, cQuadTile &tile
	, cTerrainQuadTree &terrain
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect)
{
	if (!tile.m_poi[0])
	{
		StrPath fileName = cPoiReader::GetFileName(g_mediaDir, level, xLoc, yLoc, gis::eLayerName::POI_BASE);
		if (fileName.IsFileExist())
		{
			m_loader2.LoadParallel<cPoiReader>(renderer, fileName.c_str(), &tile.m_loadFlag[gis::eLayerName::POI_BASE]
				, (void**)&tile.m_poi[0], true);
		}
		else
		{
			m_geoDownloader.DownloadFile(level, xLoc, yLoc, 0, gis::eLayerName::POI_BASE, *this, this);
		}
	}

	if (!tile.m_poi[1])
	{
		StrPath fileName = cPoiReader::GetFileName(g_mediaDir, level, xLoc, yLoc, gis::eLayerName::POI_BOUND);
		if (fileName.IsFileExist())
		{
			m_loader2.LoadParallel<cPoiReader>(renderer, fileName.c_str(), &tile.m_loadFlag[gis::eLayerName::POI_BOUND]
				, (void**)&tile.m_poi[1], true);
		}
		else
		{
			m_geoDownloader.DownloadFile(level, xLoc, yLoc, 0, gis::eLayerName::POI_BOUND, *this, this);
		}
	}

	return true;
}


// 건물인덱스 정보를 로딩한다.
bool cQuadTileManager::LoadFacilityIndex(graphic::cRenderer &renderer
	, cQuadTile &tile
	, cTerrainQuadTree &terrain
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect)
{
	if (tile.m_facilityIndex)
		return true;

	StrPath fileName = cReal3DModelIndexReader::GetFileName(g_mediaDir, level, xLoc, yLoc);
	if (fileName.IsFileExist())
	{
		tile.m_modelLoadState = 2;

		//sDistanceCompare compData;
		//compData.lonLat = gis::TileLoc2WGS84(level, xLoc, yLoc);
		m_loader2.LoadParallel<cReal3DModelIndexReader>(renderer, fileName.c_str()
			, &tile.m_loadFlag[gis::eLayerName::FACILITY_BUILD]
			, (void**)&tile.m_facilityIndex, false);
	}
	else
	{
		if (level == 15)
		{
			tile.m_modelLoadState = 1;
			//m_geoDownloader.DownloadFile(level, xLoc, yLoc, 0, gis::eLayerName::FACILITY_BUILD, *this, this);
		}
	}

	return true;
}


// 부모레벨의 파일을 먼저 로드하고, 차례대로 자식 레벨의 파일을 로딩한다.
// 부모레벨의 파일은 여러 노드에 공통되기 때문에, 파일 하나로 많은 타일을 업데이트 할 수 있어, 
// 반응성이 높아진다.
void cQuadTileManager::DownloadResourceFile(cQuadTile &tile
	, cTerrainQuadTree &terrain
	, const int level, const int xLoc, const int yLoc
	, const gis::eLayerName::Enum layerName )
{
	int lv = level;
	int x = xLoc;
	int y = yLoc;
	sQuadTreeNode<sQuadData> *parentNode = NULL;

	// 부모 레벨의 노드를 찾는다.
	// 최대 -3 레벨 부모의 노드를 찾는다.
	do
	{
		--lv;
		x >>= 1;
		y >>= 1;
		sQuadTreeNode<sQuadData> *p = terrain.m_qtree.GetNode(lv, x, y);

		if (gis::eLayerName::TILE == layerName)
		{
			if (p && p->data.tile && p->data.tile->m_texture)
				parentNode = p;
		}
		else if (gis::eLayerName::DEM == layerName)
		{
			if (p && p->data.tile && p->data.tile->m_hmap)
				parentNode = p;
		}
		else
		{
			assert(0);
		}

	} while ((lv > 0) && !parentNode);

	//return parentNode;
}


// 텍스쳐 파일이 없다면, 더 높은 레벨의 텍스쳐를 이용하고, uv를 재조정한다.
bool cQuadTileManager::ReplaceParentTexture(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain
	, cQuadTile &tile
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect
	, const int texType //0=texture, 1=heightmap
	, const bool existFile
)
{
	cQuadTile *parentTile = GetReplaceNode(terrain, level, xLoc, yLoc, texType);

	// 상위 레벨의 텍스쳐를 가져 온 후, uv 좌표를 보정한다.
	if (!parentTile)
		return false;

	// rect 와 3d 상의 rect 는 z축이 뒤바뀐 상태다. 
	// 그래서, bottom 이 top 이되고, top 이 bottom 이 된다.
	// uv 계산시 발생하는 음수는 항상 양수로 계산하면 문제 없다.
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
		//tile.m_replaceTex = NULL;
		//tile.m_replaceTexLv = (parentNode->data.tile->m_replaceTexLv < 0) ?
		//	parentNode->level : parentNode->data.tile->m_replaceTexLv;
		tile.m_replaceTexLv = parentTile->m_level;
	}
	else if (1 == texType) // heightmap
	{
		tile.m_huvs[0] = u1;
		tile.m_huvs[1] = v1;
		tile.m_huvs[2] = u2;
		tile.m_huvs[3] = v2;
		tile.m_replaceHmap = NULL;

		cHeightmap2 *replaceHmap = parentTile->m_hmap? 
			parentTile->m_hmap : parentTile->m_deepCpyHmap;
		if (!tile.m_hmap && replaceHmap)
		{
			if (m_isDeepCopySmooth && !existFile)
			{
				sHeightmapArgs2 args;
				memcpy(args.huvs, tile.m_huvs, sizeof(args.huvs));
				args.level = level;
				args.xLoc = xLoc;
				args.yLoc = yLoc;
				const StrPath fileName = cHeightmap2::GetFileName(g_mediaDir
					, level, xLoc, yLoc) + "DeepCpy";

				m_loader1.LoadParallel<cHeightmap2>(renderer
					, replaceHmap, fileName.c_str()
					, args, &tile.m_loadFlag[gis::eLayerName::DEM_CPY]
					, (void**)&tile.m_deepCpyHmap, true);
			}

			tile.m_replaceHmap = replaceHmap;
			tile.m_replaceHeightLv = parentTile->m_level;
		}

	}

	return true;
}


// texType에 따라, 대체 노드를 리턴한다.
// texType : 0=texture, 1=heightmap
cQuadTile* cQuadTileManager::GetReplaceNode(
	cTerrainQuadTree &terrain
	, const int level, const int xLoc, const int yLoc
	, const int texType
)
{
	int lv = level;
	int x = xLoc;
	int y = yLoc;
	cQuadTile *parentTile = NULL;

	// 부모 레벨의 노드를 찾는다.
	do
	{
		--lv;
		x >>= 1;
		y >>= 1;
		//sQuadTreeNode<sQuadData> *p = terrain.m_qtree.GetNode(lv, x, y);
		cQuadTile *tile = FindTile(lv, x, y);

		if (0 == texType) // texture
		{
			if (tile && tile->m_texture)
				parentTile = tile;
		}
		else if (1 == texType) // heightmap
		{
			if (tile && (tile->m_hmap || tile->m_deepCpyHmap))
				parentTile = tile;
		}
		else
		{
			assert(0);
		}

	} while ((lv > 0) && !parentTile);

	return parentTile;
}


void cQuadTileManager::Clear()
{
	m_readyLoadModel.clear();
	m_geoDownloader.Clear();

	m_loader1.ClearFile<cTileTexture>();
	m_loader1.ClearFile<cHeightmap2>();
	m_loader2.ClearFile<cPoiReader>();
	m_loader2.ClearFile<cReal3DModelIndexReader>();
	m_loader2.ClearFile<cXdoReader>();

	m_geoDownloader.Clear();
	m_heights.clear();

	for (auto &kv : m_tiles)
		delete kv.second.tile;
	m_tiles.clear();
}


// 다운로드가 끝난 파일은 cGeoDownloader::UpdateDownload() 로부터 함수가 호출된다.
void cQuadTileManager::OnDownloadComplete(const gis::sDownloadData &data)
{
	m_downloadFiles.push_back(data);
}
