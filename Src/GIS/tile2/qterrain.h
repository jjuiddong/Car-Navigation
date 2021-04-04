//
// 2021-03-29, jjuiddong
// seamless terrain renderer
//	- upgrade cTerrainQuadTree
//	- esri arcgis renderer
//	- bingmap renderer
//	- vworld renderer
//
#pragma once


namespace gis2
{

	struct sTileData
	{
		cQTile *tile; // reference
	};


	// quad tree terrain
	class cQTerrain
	{
	public:
		cQTerrain();
		virtual ~cQTerrain();

		bool Create(graphic::cRenderer &renderer);
		void Update(graphic::cRenderer &renderer, const Vector2d &camLonLat, const float deltaSeconds);
		void Render(graphic::cRenderer &renderer
			, const float deltaSeconds
			, const graphic::cFrustum &frustum
			, const Ray &ray
			, const Ray &mouseRay);
		std::pair<bool, Vector3> Pick(const Ray &ray);
		Vector3 Get3DPos(const Vector2d &lonLat);
		Vector2d GetWGS84(const Vector3 &relPos);
		void Clear();


	protected:
		void BuildQuadTree(const graphic::cFrustum &frustum, const Ray &ray);
		void RenderTessellation(graphic::cRenderer &renderer
			, const float deltaSeconds
			, const graphic::cFrustum &frustum);
		void RenderQuad(graphic::cRenderer &renderer
			, const float deltaSeconds
			, const graphic::cFrustum &frustum
			, const Ray &ray);
		void RenderRect3D(graphic::cRenderer &renderer
			, const float deltaSeconds
			, const sRectf &rect
			, const graphic::cColor &color
		);
		bool IsContain(const graphic::cFrustum &frustum, const sRectf &rect
			, const float maxH);
		int GetLevel(const float distance);


	public:
		struct sStackData
		{
			sRectf rect;
			int level;
			sQNode<sTileData> *node;
		};
		enum {MAX_STACK = 1024};

		cQuadTree2<sTileData> m_qtree;
		cTileManager m_tileMgr;
		graphic::cMaterial m_mtrl;
		graphic::cShader11 m_shader;
		graphic::cText m_text;
		Vector3 m_txtColor;
		sQNode<sTileData> *m_nodeBuffer;

		bool m_showQuadTree;
		bool m_showWireframe;
		bool m_isShowLevel;
		bool m_isShowLoc;
		int m_distanceLevelOffset;
		sStackData *m_stack; // recursive call stack
	};

}
