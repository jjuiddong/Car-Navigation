//
// 2021-03-30, jjuiddong
// Heightmap class
//	- upgrade cHeightmap2
//	- ArcGis heightmap
//
#pragma once


namespace gis2
{

	class cHeightmap3 : public graphic::iParallelLoadObject
	{
	public:
		cHeightmap3();
		cHeightmap3(graphic::cRenderer &renderer, const char *fileName);
		cHeightmap3(graphic::cRenderer &renderer, const cHeightmap3 *src
			, const char *fileName, const sHeightmapArgs2 &args);
		virtual ~cHeightmap3();
		virtual const char* Type() override { return "cHeightmap3"; }


		bool Create(graphic::cRenderer &renderer, const cHeightmap3 &src
			, const float huvs[4]
			, const int level, const int x, const int y);

		bool Read(const char *fileName);
		bool Read(const char *directoryName, const int level, const int x, const int y);
		bool ReadCustom(const char *fileName);
		bool Write(const StrPath &directoryName);

		bool ReadAndCreateTexture(graphic::cRenderer &renderer,
			const char *fileName);

		bool IsLoaded() const;
		float GetHeight(const Vector2 &uv);
		StrPath GetFileName(const char *directoryName);
		static StrPath GetFileName(const StrPath &directoryName
			, const int level, const int x, const int y);
		void Clear();


	protected:
		bool CalcBoundingBox();
		bool UpdateLocationByString(const string &str);


	public:
		int m_level;
		int m_x;
		int m_y;
		uint m_width;
		uint m_height;
		float m_maxH; // maximum height
		float m_minH; // minimum height
		float *m_data; // m_width x m_height x sizeof(float)
		bool m_isTextureUpdate;
		D3D11_MAPPED_SUBRESOURCE m_desc;
		graphic::cTexture *m_texture; // heightmap texture
		string m_fileName;
		static const float DEFAULT_H;
	};


	inline bool cHeightmap3::IsLoaded() const { return m_data && m_isTextureUpdate; }
}

