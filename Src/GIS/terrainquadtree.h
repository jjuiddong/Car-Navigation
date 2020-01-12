//
// 2017-12-12, jjuiddong
// QuadTree for Terrain
// (no multithread safe!!)
//
#pragma once

class cQuadTile;

struct sQuadData
{
	sQuadData();

	int level[4]; // eDirection index, Adjacent Quad Level
	cQuadTile *tile; // reference
};


class cTerrainQuadTree
{
public:
	cTerrainQuadTree();
	cTerrainQuadTree(sRectf &rect);
	virtual ~cTerrainQuadTree();

	bool Create(graphic::cRenderer &renderer, const bool isResDistriubution);

	void Update(graphic::cRenderer &renderer, const Vector2d &camLonLat, const float deltaSeconds);

	void Render(graphic::cRenderer &renderer
		, const float deltaSeconds
		, const graphic::cFrustum &frustum
		, const Ray &ray
		, const Ray &mouseRay);

	Vector2d GetLongLat(const Ray &ray);
	Vector3 Get3DPos(const Vector2d &lonLat);
	Vector3 Get3DPosPrecise(graphic::cRenderer &renderer, const Vector2d &lonLat);
	Vector2d GetWGS84(const Vector3 &relPos);
	float GetHeight(const Vector2 &relPos);
	inline int GetLevel(const float distance);
	std::pair<bool,Vector3> Pick(const Ray &ray);
	void Clear();


protected:
	void CalcSeamlessLevel();
	void CalcQuadEdgeLevel(sQuadTreeNode<sQuadData> *from, sQuadTreeNode<sQuadData> *to
		, const eDirection::Enum type);
	eDirection::Enum GetOpposite(const eDirection::Enum type);
	void BuildQuadTree(const graphic::cFrustum &frustum, const Ray &ray);
	void RenderTessellation(graphic::cRenderer &renderer, const float deltaSeconds, const graphic::cFrustum &frustum);
	void RenderFacility(graphic::cRenderer &renderer, const float deltaSeconds, const graphic::cFrustum &frustum);
	void RenderQuad(graphic::cRenderer &renderer, const float deltaSeconds, const graphic::cFrustum &frustum, const Ray &ray);
	void RenderResDistribution(graphic::cRenderer &renderer, const float deltaSeconds, const graphic::cFrustum &frustum, const int resType);
	void RenderRect3D(graphic::cRenderer &renderer, const float deltaSeconds, const sRectf &rect, const graphic::cColor &color);
	inline bool IsContain(const graphic::cFrustum &frustum, const sRectf &rect, const float maxH);
	bool ReadResourceTDistribution(const char *fileName);
	bool ReadResourceHDistribution(const char *fileName);
	//float GetMaximumHeight(const sQuadTreeNode<sQuadData> *node);


public:
	cQuadTree<sQuadData> m_qtree;
	static cQuadTileManager *m_tileMgr;
	graphic::cMaterial m_mtrl;
	graphic::cVertexBuffer m_vtxBuff;
	graphic::cShader11 m_shader;
	graphic::cText m_text;
	Vector3 m_txtColor;
	int m_distributeType; //0=texture, 1=heightmap
	vector<Vector3> m_resTDistribute; // texture
	vector<Vector3> m_resHDistribute; // heightmap
	sQuadTreeNode<sQuadData> *m_nodeBuffer; // reserved tree nodes

	bool m_isShowTexture;
	bool m_isShowFacility;
	bool m_isShowQuadTree;
	bool m_isShowLevel;
	bool m_isShowLoc;
	bool m_isShowPoi1;
	bool m_isShowPoi2;
	bool m_isShowDistribute;
	bool m_isLight;
	bool m_isCalcResDistribution;
	bool m_isRenderOptimize;
	int m_optimizeLevel;
	int m_showQuadCount;
	int m_loopCount;
	const char *m_techName[5]; // Light, NoTexture, Heightmap, Light_Heightmap, Unlit
	int m_techniqType;
	int m_fps;
	float m_calcOptimizeTime;
	int m_distanceLevelOffset;
	common::cTimer m_timer; // for analyze rendering
	double m_t0; // buildtree time
	double m_t1; // render tesselation time
	double m_t2; // update time
};
