//
// 2018-01-09, jjuiddong
// cQuadTileManager
//	- MemoryPool Manager
//
#pragma once


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

	float GetMaximumHeight(const int level, const int xLoc, const int yLoc);
	
	bool LoadResource(graphic::cRenderer &renderer
		, cTerrainQuadTree &terrain
		, cQuadTile &tile
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

	bool Smooth(graphic::cRenderer &renderer
		, cTerrainQuadTree &terrain
		, sQuadTreeNode<sQuadData> *node);

	bool LoadHeightMapDirect(graphic::cRenderer &renderer
		, cQuadTile &tile
		, cTerrainQuadTree &terrain
		, const int level, const int xLoc, const int yLoc
		, const sRectf &rect);

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
		, const int texType
		, const bool existFile);

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
	const double LIMIT_TIME = 0.8f; // 1.f, m_tiles>1000: 1.f, 2000>0.5f
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
	map<int64, float> m_heights; // key = cQuadTree<sQuadData>::MakeKey(level, xLoc, yLoc)
	graphic::cFileLoader2<2000, 10, sHeightmapArgs2> m_loader1; // texture, heightmap
	graphic::cFileLoader2<10000, 1> m_loader2; // poi, index, xdo

	gis::cGeoDownloader m_geoDownloader;
	vector<gis::sDownloadData> m_downloadFiles;

	// delay loading
	float m_timeSortingLoadModel;
	float m_timeLoadModel;
	double m_timeLoadBalancing;
	int m_cntLoadModelOneSecond;
	int m_cntRemoveTile;
	vector<sReadyLoadModel> m_readyLoadModel;

	// delay load file sorting
	float m_timeLoadFileSorting;

	// calc deep copy smooth
	bool m_isDeepCopySmooth;
};
