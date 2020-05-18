//
// 2017-12-19, jjuiddong
// QuadTree, Tile
//
#pragma once


class cQuadTile
{
public:
	cQuadTile();
	virtual ~cQuadTile();

	bool Create(graphic::cRenderer &renderer, const int level, const int xLoc, const int yLoc
		, const sRectf &rect
		, const char *textureFileName);

	void Render(graphic::cRenderer &renderer
		, const float deltaSeconds
		, const int adjLevel[4]
		, const XMMATRIX &tm = graphic::XMIdentity);

	void RenderFacility(graphic::cRenderer &renderer
		, const float deltaSeconds
		, const XMMATRIX &tm = graphic::XMIdentity);

	void RenderPoi(graphic::cRenderer &renderer
		, const float deltaSeconds
		, const bool isShowPoi1
		, const bool isShowPoi2
		, const XMMATRIX &tm = graphic::XMIdentity);

	bool Smooth(cQuadTile &tile, const eDirection::Enum direction);

	void SmoothPostProcess(cQuadTile &southWestTile);

	void UpdateTexture(graphic::cRenderer &renderer);

	float GetHeight(const Vector2 &uv) const;

	std::pair<bool,Vector3> Pick(const Ray &ray);

	void Clear();


public:
	void RenderGeometry(graphic::cRenderer &renderer
		, const float deltaSeconds
		, const int adjLevel[4]
		, const XMMATRIX &tm = graphic::XMIdentity);

	float GetHeightByReplaceMap(cHeightmap2 *hmap, const Vector2 &uv) const;


public:
	bool m_loaded;
	int m_renderFlag;
	int m_modelLoadState; // 0:none, 1=index file load, 2=index file read
						  // , 3=model file read/download, 4=model file read
						  // , 5=finish read file
	int m_level;
	Vector2i m_loc; // x,y Location in Quad Grid Coordinate in specific level
	sRectf m_rect; // 3d space rect
	int m_replaceTexLv; // replace texture level (upper level), default: -1
	int m_replaceHeightLv; // replace heightmap level (upper level), default: -1
	float m_tuvs[4]; // texture left-top uv, right-bottom uv coordinate
	float m_huvs[4]; // heightmap left-top uv, right-bottom uv coordinate
	cTileTexture *m_texture; // reference
	cTileTexture *m_replaceTex; // reference
	cHeightmap2 *m_hmap; // reference
	cHeightmap2 *m_deepCpyHmap; // refrence
	cHeightmap2 *m_replaceHmap; // reference
	cPoiReader *m_poi[2]; // reference, 0: poi_base, 1: poi_bound
	vector<cXdoReader*> m_facilities; // reference
	vector<cTileTexture*> m_facilitiesTex; // reference
	vector<graphic::cModel> m_mods;

	cReal3DModelIndexReader *m_facilityIndex; // reference
	graphic::cVertexBuffer *m_vtxBuff; // reference
	int m_loadFlag[gis::eLayerName::MAX_LAYERNAME]; // external reference = 1 (외부 쓰레드에서 참조되고 있을 때 1, 메모리 관리를 위해 쓰인다.)
};

