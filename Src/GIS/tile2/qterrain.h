//
// 2021-03-29, jjuiddong
// quadtree terrain renderer
//	- upgrade cTerrainQuadTree
//	- vworld renderer
//	- esri arcgis renderer
//	- bingmap renderer
//
#pragma once

#include "quadtree2.h"


namespace gis
{

	class cTile;
	struct sTileData
	{
		cTile *tile; // reference
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
		void Clear();


	protected:
		void BuildQuadTree(const graphic::cFrustum &frustum, const Ray &ray);
		bool IsContain(const graphic::cFrustum &frustum, const sRectf &rect
			, const float maxH);


	public:
		struct sStackData
		{
			sRectf rect;
			int level;
			sQNode<sTileData> *node;
		};
		enum {MAX_STACK = 1024};

		cQuadTree2<sTileData> m_qtree;
		graphic::cMaterial m_mtrl;
		graphic::cVertexBuffer m_vtxBuff;
		graphic::cShader11 m_shader;
		graphic::cText m_text;
		Vector3 m_txtColor;
		sQNode<sTileData> *m_nodeBuffer;

		bool m_showQuadTree;
		bool m_showWireframe;
		bool m_isShowLevel;
		bool m_isShowLoc;
		sStackData *m_stack; // recursive call stack
	};

}
