
#include "stdafx.h"
#include "heightmap_old.h"

using namespace graphic;

cHeightmap2::cHeightmap2()
	: m_level(0)
	, m_xLoc(0)
	, m_yLoc(0)
	, m_width(0)
	, m_height(0)
	, m_scale(0.0001f)
	, m_data(NULL)
	, m_flags(0)
{
}

cHeightmap2::cHeightmap2(graphic::cRenderer &renderer, const char *fileName)
	: m_level(0)
	, m_xLoc(0)
	, m_yLoc(0)
	, m_width(0)
	, m_height(0)
	, m_scale(0.0001f)
	, m_data(NULL)
	, m_flags(0)
{
	if (!ReadAndCreateTexture(renderer, fileName))
		throw std::exception();
}

cHeightmap2::cHeightmap2(graphic::cRenderer &renderer, const cHeightmap2 *src, const char *fileName
	, const sHeightmapArgs2 &args)
	: m_level(0)
	, m_xLoc(0)
	, m_yLoc(0)
	, m_width(0)
	, m_height(0)
	, m_scale(0.0001f)
	, m_data(NULL)
	, m_flags(0)
{
	if (!Create(renderer, *src, args.huvs, args.level, args.xLoc, args.yLoc))
		throw std::exception();
	m_fileName = fileName;
}


cHeightmap2::~cHeightmap2()
{
	Clear();
}


// src 높이맵을 복사한다. 
// 같은 너비이거나, 
// 큰 너비의 높이맵을, 작은 너비의 높이맵에 복사할 때, 이 함수를 호출해야한다.
bool cHeightmap2::Create(cRenderer &renderer, const cHeightmap2 &src, const float huvs[4]
	, const int level, const int xLoc, const int yLoc)
{
#define MACRO_SWITCH32(val, expr) \
switch (val) \
{ \
case 32: expr; \
case 31: expr; \
case 30: expr; \
case 29: expr; \
case 28: expr; \
case 27: expr; \
case 26: expr; \
case 25: expr; \
case 24: expr; \
case 23: expr; \
case 22: expr; \
case 21: expr; \
case 20: expr; \
case 19: expr; \
case 18: expr; \
case 17: expr; \
case 16: expr; \
case 15: expr; \
case 14: expr; \
case 13: expr; \
case 12: expr; \
case 11: expr; \
case 10: expr; \
case 9: expr; \
case 8: expr; \
case 7: expr; \
case 6: expr; \
case 5: expr; \
case 4: expr; \
case 3: expr; \
case 2: expr; \
case 1: expr; \
	break; \
}


	Clear();

	m_data = new float[src.m_width * src.m_height];
	ZeroMemory(m_data, src.m_width * src.m_height * sizeof(float));

	const int srcPitch = src.m_width;
	const int dstPitch = src.m_width;
	const int srcX = (int)(huvs[0] * src.m_width);
	const int srcY = (int)(huvs[1] * src.m_height);
	const int endSrcX = (int)(huvs[2] * src.m_width);
	const int endSrcY = (int)(huvs[3] * src.m_height);
	const int incX = (int)(1.f / abs(huvs[0] - huvs[2]));
	const int incY = (int)(1.f / abs(huvs[1] - huvs[3]));

	int dy = 0;
	int sy = srcY;
	while (sy < endSrcY)
	{
		int dx = 0;
		int sx = srcX;
		while (sx < endSrcX)
		{
			const float val = src.m_data[sy*srcPitch + sx];
			//m_data[dy*dstPitch + dx] = val;
			//dx += incX;
			MACRO_SWITCH32(incX, m_data[dy*dstPitch + dx++] = val);
			sx++;
		}
		//dy += incY;
		//memcpy(&m_data[dy*dstPitch], &m_data[(dy - 1)*dstPitch], sizeof(float)*dstPitch); ++dy;
		MACRO_SWITCH32(incY - 1, memcpy(&m_data[(dy+1)*dstPitch], &m_data[dy*dstPitch], sizeof(float)*dstPitch); ++dy);
		dy++;
		sy++;
	}

	//for (int y = 0; y < src.m_height; ++y)
	//{
	//	for (int x = 0; x < src.m_width - 1; ++x) // ignore outer memory
	//	{
	//		const int srcIdx = (int)(y*yScale + srcY)*srcPitch + (int)(x*xScale + srcX) + 1;
	//		m_data[y*dstPitch + x + 1] = src.m_data[srcIdx];
	//	}
	//}

	//const float srcX = huvs[0] * (src.m_width-1); // src.m_width-1, ignore outer memory
	//const float srcY = huvs[1] * (src.m_height-1); // src.m_height-1, ignore outer memory
	//const float xScale = abs(huvs[0] - huvs[2]);
	//const float yScale = abs(huvs[1] - huvs[3]);

	//for (int y = 0; y < src.m_height; ++y)
	//{
	//	for (int x = 0; x < src.m_width-1; ++x) // ignore outer memory
	//	{
	//		const int srcIdx = (int)(y*yScale + srcY)*srcPitch + (int)(x*xScale + srcX) + 1;
	//		m_data[y*dstPitch + x + 1] = src.m_data[srcIdx];
	//	}
	//}

	const bool result = m_texture.Create(renderer, src.m_width, src.m_height
		, DXGI_FORMAT_R32_FLOAT
		, m_data, src.m_width * sizeof(float)
		, D3D11_USAGE_DYNAMIC
	);

	m_level = level;
	m_xLoc = xLoc;
	m_yLoc = yLoc;
	m_width = src.m_width;
	m_height = src.m_height;

	return result;
}


// *.bil 파일을 읽는다.
// 파일이름으로 level, xLoc, yLoc을 얻어온다.
// 파일명 포맷 : *//level//yloc//yloc_xloc.bil
bool cHeightmap2::Read(const char *fileName)
{
	Clear();

	const int fileSize = (int)common::FileSize(fileName);
	RETV(fileSize <= 0, false);

	const int mapSize = (int)sqrt(fileSize / 2);
	RETV(mapSize <= 0, false);

	std::ifstream ifs(fileName, std::ios::binary);
	if (!ifs.is_open())
		return false;

	short *temp = new short[mapSize * mapSize];
	ZeroMemory(temp, mapSize * mapSize * sizeof(short));
	ifs.read((char*)temp, mapSize * mapSize * sizeof(short));

	// +1 outer memory size
	m_data = new float[(mapSize+1) * (mapSize+1)];
	ZeroMemory((char*)m_data, (mapSize + 1) * (mapSize + 1) * sizeof(float));
	for (int y = 0; y < mapSize; y++)
		for (int x = 0; x < mapSize; x++)
			m_data[y*(mapSize+1) + x+1] = ((float)temp[y*mapSize + x]) * m_scale;

	delete[] temp;
	m_width = mapSize + 1;// +1 outer memory size
	m_height = mapSize + 1;// +1 outer memory size
	m_fileName = fileName;

	{
		// 파일명에서 level, xLoc, yLoc을 추출한다.
		// *//level//yloc//yloc_xloc.bil
		vector<string> out;
		common::tokenizer(fileName, "\\", {}, out);

		if (out.size() >= 4)
		{
			const int s = out.size();
			const int level = atoi(out[s-3].c_str());
			int yLoc = 0, xLoc = 0;
			const int cnt = sscanf(out[s-1].c_str(), "%d_%d.bill", &yLoc, &xLoc);
			RETV(cnt < 2, false);

			m_level = level;
			m_xLoc = xLoc;
			m_yLoc = yLoc;
		}
	}

	Resize(256);

	return true;
}


// directoryName 위치의 level, xLoc, yLoc 위치의 높이맵 파일을 읽느다.
bool cHeightmap2::Read(const char *directoryName, const int level, const int xLoc, const int yLoc)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.bil", directoryName, level, yLoc, yLoc, xLoc);
	return Read(path.c_str());
}


// 높이맵 파일 저장 (short type)
bool cHeightmap2::Write(const char *fileName)
{
	RETV(!IsLoaded(), false);

	std::ofstream ofs(fileName, std::ios::binary);
	if (!ofs.is_open())
		return false;

	// -1 remove outer memory
	short *temp = new short[(m_width-1) * (m_height-1)];
	for (int y = 0; y < m_height-1; y++)
		for (int x = 0; x < m_width-1; x++)
			temp[y*(m_width-1) + x] = (short)(m_data[y*m_width + x] / m_scale);

	ofs.write((char*)temp, (m_width-1) * (m_height-1) * sizeof(short));
	delete[] temp;
	return true;
}


// 자동으로 파일명을 생성해서, directoryName 위치에 저장한다.
bool cHeightmap2::WriteAutoFileName(const char *directoryName)
{
	RETV(!IsLoaded(), false);
	
	using namespace std;
	StrPath path1;
	path1.Format("%s\\%d", directoryName, m_level);
	CreateDirectoryA(path1.c_str(), NULL);

	StrPath path2;
	path2.Format("%s\\%d\\%04d", directoryName, m_level, m_yLoc);
	CreateDirectoryA(path2.c_str(), NULL);

	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.bil", directoryName, m_level, m_yLoc, m_yLoc, m_xLoc);
	return Write(path.c_str());
}


// 높이맵 파일을 읽고, 생성한다.
bool cHeightmap2::ReadAndCreateTexture( graphic::cRenderer &renderer, const char *fileName )
{
	if (!Read(fileName))
		return false;

	const bool result = m_texture.Create(renderer, m_width, m_height
		, DXGI_FORMAT_R32_FLOAT
		, m_data, m_width * sizeof(float)
		, D3D11_USAGE_DYNAMIC
		);

	return result;
}


// 두개의 높이맵에서 서로 붙어있는 픽셀을 중간값으로 맞춘다.
// orientation : map orientation, 0=North, 1=East, 2=South, 3=West
void cHeightmap2::Smooth(cHeightmap2 &hmap, const int orientation)
{
	if (m_level != hmap.m_level)
		return;
	if (!IsLoaded() || !hmap.IsLoaded())
		return;

	float *p1 = NULL, *p2 = NULL;
	int pitch1 = 0, pitch2 = 0;
	switch (orientation)
	{
	case 0: // North
	{
		p1 = m_data;
		p2 = hmap.m_data + (hmap.m_height - 1)*hmap.m_width;
		pitch1 = 1;
		pitch2 = 1;
	}
	break;

	case 1: // East
	{
		p1 = m_data + (m_width-1);
		p2 = hmap.m_data;
		pitch1 = m_width;
		pitch2 = hmap.m_width;
	}
	break;

	case 2: // South
	{
		p1 = m_data + (m_height - 1)*m_width;
		p2 = hmap.m_data;
		pitch1 = 1;
		pitch2 = 1;
	}
	break;

	case 3: // West
	{
		p1 = m_data;
		p2 = hmap.m_data + (hmap.m_width-1);
		pitch1 = m_width;
		pitch2 = hmap.m_width;
	}
	break;

	default:
		assert(0);
		break;
	}

	for (int i = 0; i < m_width; ++i)
	{
		const float v1 = *p1;
		const float v2 = *p2;
		*p1 = (v1 + v2) / 2;
		*p2 = (v1 + v2) / 2;
		p1 += pitch1;
		p2 += pitch2;
	}
}


// adjacent : W - NW - N - NE - E - SE - S - SW
void cHeightmap2::SmoothVtx(cHeightmap2 adjacent[8])
{
	RET(!IsLoaded());

	//*------*------*------*
	//|      |      |      |
	//|   1  |   2  |  3   |
	//|      |      |      |
	//*------*------*------*
	//|      |      |      |
	//|  0   | this |  4   |
	//|      |      |      |
	//*------*------*------*
	//|      |      |      |
	//|  7   |  6   |  5   |
	//|      |      |      |
	//*------*------*------*

	// LeftTop
	{
		int cnt = 1;
		float value = GetLeftTop();
		if (adjacent[0].IsLoaded())
		{
			value += adjacent[0].GetRightTop();
			++cnt;
		}
		if (adjacent[1].IsLoaded())
		{
			value += adjacent[1].GetRightBottom();
			++cnt;
		}
		if (adjacent[2].IsLoaded())
		{
			value += adjacent[2].GetLeftBottom();
			++cnt;
		}
		value /= cnt; // Average

		SetLeftTop(value);
		if (adjacent[0].IsLoaded())
			adjacent[0].SetRightTop(value);
		if (adjacent[1].IsLoaded())
			adjacent[1].SetRightBottom(value);
		if (adjacent[2].IsLoaded())
			adjacent[2].SetLeftBottom(value);
	}

	// RightTop
	// RightBottom

	// LeftBottom
	{
		int cnt = 1;
		float value = GetLeftBottom();
		if (adjacent[0].IsLoaded())
		{
			value += adjacent[0].GetRightBottom();
			++cnt;
		}
		if (adjacent[7].IsLoaded())
		{
			value += adjacent[7].GetRightTop();
			++cnt;
		}
		if (adjacent[6].IsLoaded())
		{
			value += adjacent[6].GetLeftTop();
			++cnt;
		}
		value /= cnt; // Average

		SetLeftBottom(value);
		if (adjacent[0].IsLoaded())
			adjacent[0].SetRightBottom(value);
		if (adjacent[7].IsLoaded())
			adjacent[7].SetRightTop(value);
		if (adjacent[6].IsLoaded())
			adjacent[6].SetLeftTop(value);
	}
}


// 높이맵을 상수배로 확대하거나, 축소한다.
void cHeightmap2::Resize(const int size)
{
	const int nw = size;
	const int nh = size;
	float *temp = new float[nw * nh];
	ZeroMemory(temp, sizeof(float) * nw * nh);
	const float scale = (float)(m_width-1) / (float)(size-1); // ignore outer memory
	
	for (int y = 0; y < nh-1; ++y) // ignore outer memory
	{
		for (int x = 0; x < nw-1; ++x) // ignore outer memory
		{
			const int srcIdx = (int)(y*scale) * m_width + (int)(x*scale) + 1; // ignore outer memory
			temp[y*nh + x + 1] = m_data[srcIdx];
		}
	}

	delete[] m_data; 
	m_data = temp;
	m_width = nw;
	m_height = nh;
}


StrPath cHeightmap2::GetFileName(const char *directoryName
	, const int level, const int xLoc, const int yLoc)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.bil", directoryName, level, yLoc, yLoc, xLoc);
	return path;
}


StrPath cHeightmap2::GetFileName(const char *directoryName)
{
	return GetFileName(directoryName, m_level, m_xLoc, m_yLoc);
}


void cHeightmap2::Clear()
{
	SAFE_DELETEA(m_data);
	m_texture.Clear();
}
