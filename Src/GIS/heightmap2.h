//
// 2019-12-21, jjuiddong
// Heightmap class
// read, write short type height value
// calculate float 
//
// heightmap memory structure
//	- only heightmap memory
//
#pragma once

#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>


struct sHeightmapArgs2
{
	float huvs[4];
	int level;
	int xLoc;
	int yLoc;
};


class cHeightmap2 : public graphic::iParallelLoadObject
{
public:
	cHeightmap2();
	cHeightmap2(graphic::cRenderer &renderer, const char *fileName);
	cHeightmap2(graphic::cRenderer &renderer, const cHeightmap2 *src, const char *fileName
		, const sHeightmapArgs2 &args);
	virtual ~cHeightmap2();
	virtual const char* Type() override { return "cHeightmap2"; }


	bool Create(graphic::cRenderer &renderer, const cHeightmap2 &src, const float huvs[4]
		, const int level, const int xLoc, const int yLoc);

	bool Read(const char *fileName
		, const eHeightmapFormat::Enum fmt = eHeightmapFormat::FMT_FLOAT);

	bool Read(const char *directoryName, const int level, const int xLoc, const int yLoc
		, const eHeightmapFormat::Enum fmt = eHeightmapFormat::FMT_FLOAT);

	bool ReadAndCreateTexture(graphic::cRenderer &renderer, const char *fileName
		, const eHeightmapFormat::Enum fmt = eHeightmapFormat::FMT_FLOAT);

	bool Write(const char *fileName);
	bool WriteAutoFileName(const char *directoryName);
	void EdgeSmooth(const cHeightmap2 &hmap, const int orientation);
	void EdgeSmooth(const cHeightmap2 *adjacent[8]);
	void EdgeSmoothBySelf();
	void SmoothVtx(cHeightmap2 adjacent[8]);
	void Resize(const int size);
	void GaussianSmoothing();
	void UpdateTexture(graphic::cRenderer &renderer);
	void UpdateTexture(const D3D11_MAPPED_SUBRESOURCE &desc);

	bool IsLoaded() const;
	float GetHeight(const Vector2 &uv);
	StrPath GetFileName(const char *directoryName);
	static StrPath GetFileName(const StrPath &directoryName, const int level, const int xLoc, const int yLoc);
	void Clear();

	float GetLeftTop();
	float GetRightTop();
	float GetRightBottom();
	float GetLeftBottom();
	void SetLeftTop(const float value);
	void SetRightTop(const float value);
	void SetRightBottom(const float value);
	void SetLeftBottom(const float value);


protected:
	float GetMaxHeight();


public:
	int m_level;
	int m_xLoc;
	int m_yLoc;
	int m_width;
	int m_height;
	float m_maxHeight; // maximize height
	float m_scale; // default: 0.0001, 텍스쳐 입력이 0~1 사이이기 때문에 스케일링을 해야함.
	float *m_data; // m_width x m_height x sizeof(float)
	bool m_isTextureUpdate;
	int m_waitState;
	D3D11_MAPPED_SUBRESOURCE m_desc;
	graphic::cTexture *m_texture; // heightmap texture
	string m_fileName;
	static const float DEFAULT_H;
};


inline bool cHeightmap2::IsLoaded() const { return m_data && m_isTextureUpdate; }
inline float cHeightmap2::GetLeftTop() { return *m_data; }
inline float cHeightmap2::GetRightTop() { return *(m_data + m_width - 1); }
inline float cHeightmap2::GetRightBottom() { return *(m_data + (m_width * (m_height - 1)) + m_width - 1); }
inline float cHeightmap2::GetLeftBottom() { return *(m_data + (m_width * (m_height - 1))); }
inline void cHeightmap2::SetLeftTop(const float value) { *m_data = value; }
inline void cHeightmap2::SetRightTop(const float value) { *(m_data + m_width - 1) = value; }
inline void cHeightmap2::SetRightBottom(const float value) { *(m_data + (m_width * (m_height - 1)) + m_width - 1) = value; }
inline void cHeightmap2::SetLeftBottom(const float value) { *(m_data + (m_width * (m_height - 1))) = value; }
