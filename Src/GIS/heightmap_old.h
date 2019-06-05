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
//	- 1 pixel outer memory
//
//  outer memory (left adjacent heightmap, 1 pixel)
//  ----------------------
//  ::                   |
//  ::                   |
//  ::                   |
//  ::  inner heightmap  |
//  ::     memory        |
//  ::                   |
//  ::                   |
//  :::::::::::::::::::::: outer memory (bottom adjacent heightmap, 1 pixel)
//
//  (m_width + 1) x (m_height + 1)
//
#pragma once

struct sHeightmapArgs2
{
	float huvs[4];
	int level;
	int xLoc;
	int yLoc;
};


class cHeightmap2
{
public:
	cHeightmap2();
	cHeightmap2(graphic::cRenderer &renderer, const char *fileName);
	cHeightmap2(graphic::cRenderer &renderer, const cHeightmap2 *src, const char *fileName
		, const sHeightmapArgs2 &args);
	virtual ~cHeightmap2();

	bool Create(graphic::cRenderer &renderer, const cHeightmap2 &src, const float huvs[4]
		, const int level, const int xLoc, const int yLoc);
	bool Read(const char *fileName);
	bool Read(const char *directoryName, const int level, const int xLoc, const int yLoc);
	bool ReadAndCreateTexture(graphic::cRenderer &renderer, const char *fileName);

	bool Write(const char *fileName);
	bool WriteAutoFileName(const char *directoryName);
	void Smooth(cHeightmap2 &hmap, const int orientation);
	void SmoothVtx(cHeightmap2 adjacent[8]);
	void Resize(const int size);
	bool IsLoaded();
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


public:
	int m_level;
	int m_xLoc;
	int m_yLoc;
	int m_width;
	int m_height;
	float m_scale; // default: 0.0001, 텍스쳐 입력이 0~1 사이이기 때문에 스케일링을 해야함.
	float *m_data; // m_width x m_height x sizeof(float)
	graphic::cTexture m_texture; // heightmap texture
	Str64 m_fileName;
	int m_flags; // external reference = 1 (외부 쓰레드에서 참조되고 있을 때 1, 메모리 관리를 위해 쓰인다.)
};


inline bool cHeightmap2::IsLoaded() { return m_data? true : false; }

inline float cHeightmap2::GetLeftTop() { return *m_data;}
inline float cHeightmap2::GetRightTop(){ return *(m_data + m_width - 1);}
inline float cHeightmap2::GetRightBottom() {return *(m_data + (m_width * (m_height - 1)) + m_width - 1);}
inline float cHeightmap2::GetLeftBottom(){return *(m_data + (m_width * (m_height - 1)));}
inline void cHeightmap2::SetLeftTop(const float value){*m_data = value;}
inline void cHeightmap2::SetRightTop(const float value){*(m_data + m_width - 1) = value;}
inline void cHeightmap2::SetRightBottom(const float value){*(m_data + (m_width * (m_height - 1)) + m_width - 1) = value;}
inline void cHeightmap2::SetLeftBottom(const float value){*(m_data + (m_width * (m_height - 1))) = value;}
