//
// 2021-03-29, jjuiddong
// seamless terrain renderer tile manager
//	- upgrade cQuadTileManager
//	- arcgis
//
#pragma once


namespace gis2
{

	class cQTerrain;

	class cTileManager : public gis::iDownloadFinishListener
	{
	public:
		cTileManager();
		virtual ~cTileManager();

		bool Update(graphic::cRenderer &renderer
			, cQTerrain &terrain
			, const Vector2d &camLonLat
			, const float deltaSeconds);

		cQTile* GetTile(graphic::cRenderer &renderer
			, const int level, const int x, const int y
			, const sRectf &rect);

		cQTile* FindTile(const int level, const int x, const int y);

		bool LoadResource(graphic::cRenderer &renderer
			, cQTerrain &terrain
			, cQTile &tile
			, const int level, const int xLoc, const int yLoc
			, const sRectf &rect);

		void Clear();


	protected:
		void CreateVertexBuffer(graphic::cRenderer &renderer);
		void UpdateDownloadFile(graphic::cRenderer &renderer);
		void LoadTexture(graphic::cRenderer &renderer
			, cQTile &tile
			, cQTerrain &terrain
			, const int level, const int x, const int y
			, const sRectf &rect);
		bool LoadHeightMap(graphic::cRenderer &renderer
			, cQTile &tile
			, cQTerrain &terrain
			, const int level, const int x, const int y
			, const sRectf &rect);
		bool ReplaceParentTexture(graphic::cRenderer &renderer
			, cQTerrain &terrain
			, cQTile &tile
			, const int level, const int x, const int y
			, const sRectf &rect
			, const int texType //0=texture, 1=heightmap
			, const bool existFile
		);
		cQTile* GetReplaceNode(cQTerrain &terrain
			, const int level, const int x, const int y
			, const int texType
		);

		virtual void OnDownloadComplete(const gis::sDownloadData &data) override;


	public:
		const double LIMIT_TIME = 0.8f; // 1.f, m_tiles>1000: 1.f, 2000>0.5f
		struct sTileMem
		{
			double accessTime; // tile access time, if over LIMIT_TIME, remove this tile
			cQTile *tile;
		};

		cTimer m_timer;
		double m_curFrameT;

		map<__int64, sTileMem> m_tiles; // key = qtree::MakeKey(level, x, y)
		graphic::cVertexBuffer m_tileVtxBuff;
		graphic::cVertexBuffer m_tileLineVtxBuff;
		map<int64, float> m_heights; // key = qtree::MakeKey(level, x, y)
		graphic::cFileLoader2<2000, 10, sHeightmapArgs2> m_loader1; // texture, heightmap

		gis::cGeoDownloader m_geoDownloader;
		vector<gis::sDownloadData> m_downloadFiles;
	};

}
