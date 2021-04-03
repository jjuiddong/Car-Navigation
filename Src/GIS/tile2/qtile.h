//
// 2021-03-29, jjuiddong
// terrain tile
//	- terrain quad tree tile
//	- upgrade cQuadTile class
//
#pragma once


namespace gis2
{

	class cQTile
	{
	public:
		cQTile();
		virtual ~cQTile();

		bool Create(graphic::cRenderer &renderer
			, const int level, const int x, const int y
			, const sRectf &rect
			, const char *textureFileName);

		void Render(graphic::cRenderer &renderer
			, const float deltaSeconds
			, const XMMATRIX &tm = graphic::XMIdentity);

		std::pair<bool, Vector3> Pick(const Ray &ray);
		float GetHeight(const Vector2 &uv) const;
		void Clear();


	protected:
		float GetHeightByReplaceMap(cHeightmap3 *hmap, const Vector2 &uv) const;


	public:
		bool m_loaded;
		int m_renderFlag;
		int m_modelLoadState; // 0:none, 1=index file load, 2=index file read
							  // , 3=model file read/download, 4=model file read
							  // , 5=finish read file
		int m_level;
		Vector2i m_loc; // x,y Location in Quad Grid Coordinate in specific level
		sRectf m_rect; // 3d space rect
		float m_tuvs[4]; // texture left-top uv, right-bottom uv coordinate
		float m_huvs[4]; // heightmap left-top uv, right-bottom uv coordinate
		cTileTexture *m_texture; // reference
		cTileTexture *m_replaceTex; // reference
		cHeightmap3 *m_hmap; // reference
		cHeightmap3 *m_replaceHmap; // reference
		graphic::cVertexBuffer *m_vtxBuff; // reference
		int m_loadFlag[gis::eLayerName::MAX_LAYERNAME]; // external reference = 1 (access external thread to memory management)
	};

}
