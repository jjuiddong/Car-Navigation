//
// 2018-01-09, jjuiddong
// cQuadTileManager
//	- MemoryPool Manager
//
#pragma once

#include "../../../Common/Graphic11/importer/fileloader.h"
#include "vworldwebdownloader.h"

class cTerrainQuadTree;
struct sQuadData;

struct sDistanceCompare
{
	Vector2d lonLat;
};


class cQuadTileManager : public gis::iDownloadFinishListener
{
public:
	struct sReadyLoadModel {
		int level; // tile loc
		Vector2i loc; // tile loc
		Vector2d lonLat;
		int facilityIdx;
	};


	cQuadTileManager();
	virtual ~cQuadTileManager();

	bool Update(graphic::cRenderer &renderer
		, cTerrainQuadTree &terrain
		, const Vector2d &camLonLat
		, const float deltaSeconds);

	cQuadTile* GetTile(graphic::cRenderer &renderer
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

	cQuadTile* FindTile(const int level, const int xLoc, const int yLoc);

	bool LoadResource(graphic::cRenderer &renderer
		, cTerrainQuadTree &terrain
		, cQuadTile &tile
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

	bool Smooth(graphic::cRenderer &renderer
		, cTerrainQuadTree &terrain
		, sQuadTreeNode<sQuadData> *node);

	void Clear();


protected:
	void UpdateDownloadFile(graphic::cRenderer &renderer);
	void UpdateLoadModelFile(graphic::cRenderer &renderer
		, const Vector2d &camLonLat, cTerrainQuadTree &terrain
		, const float deltaSeconds);
	void SortingDownloadFile(const Vector2d &camLonLat, cTerrainQuadTree &terrain
		, const float deltaSeconds);

	void LoadTexture(graphic::cRenderer &renderer
		, cQuadTile &tile
		, cTerrainQuadTree &terrain
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

	bool LoadHeightMap(graphic::cRenderer &renderer
		, cQuadTile &tile
		, cTerrainQuadTree &terrain
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

	bool LoadPoiFile(graphic::cRenderer &renderer
		, cQuadTile &tile
		, cTerrainQuadTree &terrain
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

	bool LoadFacilityIndex(graphic::cRenderer &renderer
		, cQuadTile &tile
		, cTerrainQuadTree &terrain
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

	bool LoadFacilities(graphic::cRenderer &renderer
		, cQuadTile &tile
		, cTerrainQuadTree &terrain
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

	bool ReplaceParentTexture(graphic::cRenderer &renderer
		, cTerrainQuadTree &terrain
		, cQuadTile &tile
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect
		, const int texType);

	cQuadTile* GetReplaceNode(
		cTerrainQuadTree &terrain
		, const int level
		, const int xLoc, const int yLoc, const int texType);

	void CreateVertexBuffer(graphic::cRenderer &renderer);

	void DownloadResourceFile(cQuadTile &tile
		, cTerrainQuadTree &terrain
		, const int level, const int xLoc, const int yLoc
		, const gis::eLayerName::Enum layerName);

	bool CreateFacility(graphic::cRenderer &renderer
		, cTerrainQuadTree &terrain
		, const sReadyLoadModel &modInfo);

	virtual void OnDownloadComplete(const gis::sDownloadData &data) override;


public:
	const double LIMIT_TIME = 1.f; // m_tiles>1000: 1.f, 2000>0.5f
	struct sTileMem
	{
		double accessTime; // LIMIT_TIME이상 지나면, 메모리를 제거한다.
		cQuadTile *tile;
	};

	cTimer m_timer;
	double m_curFrameT;

	map<__int64, sTileMem> m_tiles; // key = cQuadTree<sQuadData>::MakeKey(level, xLoc, yLoc)
	graphic::cVertexBuffer m_tileVtxBuff;
	graphic::cVertexBuffer m_tileLineVtxBuff;
	graphic::cFileLoader<cTileTexture, 1000> m_tmaps;
	graphic::cFileLoader<cHeightmap, 1000, sHeightmapArgs> m_hmaps;
	graphic::cFileLoader<cPoiReader, 1000> m_pmaps[2]; // 0:poi_base, 1:poi_bound
	graphic::cFileLoader<cReal3DModelIndexReader, 3000, graphic::sFileLoaderArg, sDistanceCompare> m_modelIndices;
	graphic::cFileLoader<cXdoReader, 5000, graphic::sFileLoaderArg, sDistanceCompare> m_facilities;
	graphic::cFileLoader<cTileTexture, 5000, graphic::sFileLoaderArg, sDistanceCompare> m_facilitiesTex;

	gis::cVWorldWebDownloader m_vworldDownloader;
	vector<gis::sDownloadData> m_downloadFiles;

	// delay loading
	float m_timeSortingLoadModel;
	float m_timeLoadModel;
	double m_timeLoadBalancing;
	int m_cntLoadModelOneSecond;
	vector<sReadyLoadModel> m_readyLoadModel;

	// delay load file sorting
	float m_timeLoadFileSorting;
};
