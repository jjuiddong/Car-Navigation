//
// 2018-01-04, jjuiddong
// Heightmap class
// read, write short type height value
// calculate float 
//
// heightmap memory structure
//	- only heightmap memory
//
// heightmap texture memory structure
//	- adjacent height memory (outer) + heightmap memory (inner)
//	- left-top-right-bottom 1 pixel outer memory
//
//  :::::::::::::::::::::::
//  ::                   ::
//  ::                   ::
//  ::                   ::
//  ::  inner heightmap  ::
//  ::     memory        ::
//  ::                   ::
//  ::                   ::
//  ::::::::::::::::::::::: outer memory
//
//  (m_width + 2) x (m_height + 2)
//
#pragma once

#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>


struct eHeightmapFormat {
	enum Enum { FMT_SHORT, FMT_FLOAT };
};

struct sHeightmapArgs
{
	float huvs[4];
	int level;
	int xLoc;
	int yLoc;
};


class cHeightmap
{
public:
	cHeightmap();
	cHeightmap(graphic::cRenderer &renderer, const char *fileName);
	cHeightmap(graphic::cRenderer &renderer, const cHeightmap *src, const char *fileName
		, const sHeightmapArgs &args);
	virtual ~cHeightmap();

	bool Create(graphic::cRenderer &renderer, const cHeightmap &src, const float huvs[4]
		, const int level, const int xLoc, const int yLoc);

	bool Read(const char *fileName
		, const eHeightmapFormat::Enum fmt = eHeightmapFormat::FMT_FLOAT);

	bool Read(const char *directoryName, const int level, const int xLoc, const int yLoc
		, const eHeightmapFormat::Enum fmt = eHeightmapFormat::FMT_FLOAT);

	bool ReadAndCreateTexture(graphic::cRenderer &renderer, const char *fileName
		, const eHeightmapFormat::Enum fmt = eHeightmapFormat::FMT_FLOAT);

	bool Write(const char *fileName);
	bool WriteAutoFileName(const char *directoryName);
	void EdgeSmooth(const cHeightmap &hmap, const int orientation);
	void EdgeSmooth(const cHeightmap *adjacent[8]);
	void EdgeSmoothBySelf();
	void SmoothVtx(cHeightmap adjacent[8]);
	void Resize(const int size);
	void GaussianSmoothing();
	void UpdateTexture(graphic::cRenderer &renderer);
	void UpdateTexture(const D3D11_MAPPED_SUBRESOURCE &desc);

	bool IsLoaded() const;
	float GetHeight(const Vector2 &uv);
	StrPath GetFileName(const char *directoryName);
	static StrPath GetFileName(const char *directoryName, const int level, const int xLoc, const int yLoc);
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
	StrPath m_fileName;
};


inline bool cHeightmap::IsLoaded() const { return m_data && m_isTextureUpdate; }

inline float cHeightmap::GetLeftTop() { return *m_data;}
inline float cHeightmap::GetRightTop(){ return *(m_data + m_width - 1);}
inline float cHeightmap::GetRightBottom() {return *(m_data + (m_width * (m_height - 1)) + m_width - 1);}
inline float cHeightmap::GetLeftBottom(){return *(m_data + (m_width * (m_height - 1)));}
inline void cHeightmap::SetLeftTop(const float value){*m_data = value;}
inline void cHeightmap::SetRightTop(const float value){*(m_data + m_width - 1) = value;}
inline void cHeightmap::SetRightBottom(const float value){*(m_data + (m_width * (m_height - 1)) + m_width - 1) = value;}
inline void cHeightmap::SetLeftBottom(const float value){*(m_data + (m_width * (m_height - 1))) = value;}
