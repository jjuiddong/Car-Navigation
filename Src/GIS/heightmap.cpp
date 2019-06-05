
#include "stdafx.h"
#include "heightmap.h"


using namespace graphic;

extern common::cMemoryPool2<65 * 65 * sizeof(float)> g_memPool65;
extern common::cMemoryPool2<67 * 67 * sizeof(float)> g_memPool67;
extern common::cMemoryPool2<258 * 258 * sizeof(float)> g_memPool258;
extern common::cMemoryPool3<graphic::cTexture, 512> g_memPoolTex;


cHeightmap::cHeightmap()
	: m_level(0)
	, m_xLoc(0)
	, m_yLoc(0)
	, m_width(0)
	, m_height(0)
	, m_scale(0.00001f)
	, m_data(NULL)
	, m_maxHeight(0)
	, m_texture(NULL)
	, m_isTextureUpdate(false)
	, m_waitState(0)
{
}

cHeightmap::cHeightmap(graphic::cRenderer &renderer, const char *fileName)
	: m_level(0)
	, m_xLoc(0)
	, m_yLoc(0)
	, m_width(0)
	, m_height(0)
	, m_scale(0.00001f)
	, m_data(NULL)
	, m_texture(NULL)
	, m_isTextureUpdate(false)
	, m_waitState(0)
{
	if (!ReadAndCreateTexture(renderer, fileName))
		throw std::exception();
}

cHeightmap::cHeightmap(graphic::cRenderer &renderer, const cHeightmap *src, const char *fileName
	, const sHeightmapArgs &args)
	: m_level(0)
	, m_xLoc(0)
	, m_yLoc(0)
	, m_width(0)
	, m_height(0)
	, m_scale(0.00001f)
	, m_data(NULL)
	, m_texture(NULL)
	, m_isTextureUpdate(false)
	, m_waitState(0)
{
	if (!Create(renderer, *src, args.huvs, args.level, args.xLoc, args.yLoc))
		throw std::exception();
	m_fileName = fileName;
}


cHeightmap::~cHeightmap()
{
	Clear();
}


// src 높이맵을 복사한다. 
// 같은 너비이거나, 
// 큰 너비의 높이맵을, 작은 너비의 높이맵에 복사할 때, 이 함수를 호출해야한다.
bool cHeightmap::Create(cRenderer &renderer, const cHeightmap &src
	, const float huvs[4]
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

	//m_data = new float[src.m_width * src.m_height];
	m_data = (float*)g_memPool258.Alloc();
	if (!m_data)
	{
		dbg::Log("Error cHeightmap::Create() \n");
		return false;
	}
	ZeroMemory(m_data, src.m_width * src.m_height * sizeof(float));

	const int srcPitch = src.m_width;
	const int dstPitch = src.m_width;
	const int srcX = (int)(huvs[0] * (src.m_width-2));
	const int srcY = (int)(huvs[1] * (src.m_height-2));
	const int endSrcX = (int)(huvs[2] * (src.m_width-2));
	const int endSrcY = (int)(huvs[3] * (src.m_height-2));
	const int incX = (int)(1.f / abs(huvs[0] - huvs[2]));
	const int incY = (int)(1.f / abs(huvs[1] - huvs[3]));

	assert(srcX >= 0);
	assert(srcY >= 0);
	assert(endSrcX >= 0);
	assert(endSrcY >= 0);

	int dy = 0;
	int sy = srcY;
	while (sy < endSrcY)
	{
		int dx = 0;
		int sx = srcX;
		while (sx < endSrcX)
		{
			const float val = src.m_data[(sy+1)*srcPitch + sx + 1];
			MACRO_SWITCH32(incX, m_data[(dy+1)*dstPitch + 1 + dx++] = val);
			sx++;
		}
		MACRO_SWITCH32(incY - 1, memcpy(&m_data[(dy+2)*dstPitch], &m_data[(dy+1)*dstPitch], sizeof(float)*dstPitch); ++dy);

		dy++;
		sy++;
	}

	m_level = level;
	m_xLoc = xLoc;
	m_yLoc = yLoc;
	m_width = src.m_width;
	m_height = src.m_height;

	EdgeSmoothBySelf();
	GaussianSmoothing();
	EdgeSmoothBySelf();
	m_maxHeight = GetMaxHeight();

	if (m_texture = new graphic::cTexture())
	{
		//if (m_texture->IsLoaded())
		//{
		//	//UpdateTexture(renderer);
		//	m_waitState = 1;
		//}
		//else
		//{
			m_texture->Create(renderer, src.m_width, src.m_height
				, DXGI_FORMAT_R32_FLOAT
				, m_data, src.m_width * sizeof(float)
				, D3D11_USAGE_DYNAMIC);
			m_isTextureUpdate = true;
		//}
	}

	//const bool result = m_texture.Create(renderer, src.m_width, src.m_height
	//	, DXGI_FORMAT_R32_FLOAT
	//	, m_data, src.m_width * sizeof(float)
	//	, D3D11_USAGE_DYNAMIC
	//);

	//return result;
	return m_texture? true : false;
}


inline int GetFormatSize(const eHeightmapFormat::Enum fmt)
{
	switch (fmt)
	{
	case eHeightmapFormat::FMT_SHORT: return sizeof(short);
	case eHeightmapFormat::FMT_FLOAT: return sizeof(float);
	default: assert(0); break;
	}
	return 1;
}


// *.bil 파일을 읽는다.
// 파일이름으로 level, xLoc, yLoc을 얻어온다.
// 파일명 포맷 : *//level//yloc//yloc_xloc.bil
bool cHeightmap::Read(const char *fileName
	, const eHeightmapFormat::Enum fmt //= eHeightmapFormat::FMT_FLOAT
)
{
	Clear();

	const int fileSize = (int)common::FileSize(fileName);
	RETV(fileSize <= 0, false);

	const int mapSize = (int)sqrt(fileSize / GetFormatSize(fmt));
	RETV(mapSize != 65, false);

	std::ifstream ifs(fileName, std::ios::binary);
	if (!ifs.is_open())
		return false;

	// temporary file read memory
	//BYTE *temp = new BYTE[mapSize * mapSize * GetFormatSize(fmt)];
	BYTE *temp = (BYTE*)g_memPool65.Alloc();
	ZeroMemory(temp, mapSize * mapSize * GetFormatSize(fmt));
	if (mapSize > 50)
		ifs.read((char*)temp, mapSize * mapSize * GetFormatSize(fmt));

	//m_data = new float[(mapSize+2) * (mapSize+2)];
	m_data = (float*)g_memPool67.Alloc();
	ZeroMemory((char*)m_data, (mapSize + 2) * (mapSize + 2) * sizeof(float));

	if (fmt == eHeightmapFormat::FMT_SHORT)
	{
		short *tempShort = (short*)temp;

		// +1 outer memory size
		for (int y = 0; y < mapSize; y++)
			for (int x = 0; x < mapSize; x++)
				m_data[(y+1)*(mapSize+2) + x+1] = ((float)tempShort[y*mapSize + x]) * m_scale + 0.1f;
	}
	else if (fmt == eHeightmapFormat::FMT_FLOAT)
	{
		float *tempFloat = (float*)temp;

		// +1 outer memory size
		for (int y = 0; y < mapSize; y++)
			for (int x = 0; x < mapSize; x++)
				m_data[(y+1)*(mapSize + 2) + x + 1] = ((float)tempFloat[y*mapSize + x]) * m_scale + 0.1f;
	}
	//delete[] temp;
	g_memPool65.Free(temp);

	m_width = mapSize + 2;// +2 outer memory size
	m_height = mapSize + 2;// +2 outer memory size
	m_fileName = fileName;

	EdgeSmoothBySelf();

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

	Resize(258);
	GaussianSmoothing();
	EdgeSmoothBySelf();
	m_maxHeight = GetMaxHeight();

	return true;
}


// directoryName 위치의 level, xLoc, yLoc 위치의 높이맵 파일을 읽느다.
bool cHeightmap::Read(const char *directoryName, const int level, const int xLoc, const int yLoc
	, const eHeightmapFormat::Enum fmt //= eHeightmapFormat::FMT_FLOAT
)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.bil", directoryName, level, yLoc, yLoc, xLoc);
	return Read(path.c_str(), fmt);
}


// 높이맵 파일 저장 (short type)
bool cHeightmap::Write(const char *fileName)
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
bool cHeightmap::WriteAutoFileName(const char *directoryName)
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
bool cHeightmap::ReadAndCreateTexture( graphic::cRenderer &renderer, const char *fileName
	, const eHeightmapFormat::Enum fmt //= eHeightmapFormat::FMT_FLOAT
)
{
	if (!Read(fileName, fmt))
		return false;

	if (m_texture = new graphic::cTexture())
	{
		//if (m_texture->IsLoaded())
		//{
		//	m_waitState = 1;
		//	m_isTextureUpdate = false;

		//	//while (m_waitState == 1)
		//	//{
		//	//	//..wait
		//	//}

		//	//UpdateTexture(m_desc);
		//	//m_waitState = 3; // unmap
		//	//m_isTextureUpdate = true;
		//}
		//else
		{
			m_texture->Create(renderer, m_width, m_height
				, DXGI_FORMAT_R32_FLOAT
				, m_data, m_width * sizeof(float)
				, D3D11_USAGE_DYNAMIC);

			m_isTextureUpdate = true;
		}
	}

	//const bool result = m_texture.Create(renderer, m_width, m_height
	//	, DXGI_FORMAT_R32_FLOAT
	//	, m_data, m_width * sizeof(float)
	//	, D3D11_USAGE_DYNAMIC
	//	);

	//return result;
	return m_texture ? true : false;
}


// 두개의 높이맵에서 서로 붙어있는 픽셀을 중간값으로 맞춘다.
// orientation : 
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
void cHeightmap::EdgeSmooth(const cHeightmap &hmap, const int orientation)
{
	if (m_level != hmap.m_level)
		return;
	if (!IsLoaded() || !hmap.IsLoaded())
		return;

	float *dst = NULL, *src = NULL;
	int pitch1 = 0, pitch2 = 0;
	switch (orientation)
	{
	case 0: // West
	{
		dst = m_data + m_width;
		src = hmap.m_data + hmap.m_width + (hmap.m_width-2);
		pitch1 = m_width;
		pitch2 = hmap.m_width;
	}
	break;

	case 1: // North-West
		break;

	case 2: // North
	{
		dst = m_data + 1;
		src = hmap.m_data + (hmap.m_height - 2)*hmap.m_width + 1;
		pitch1 = 1;
		pitch2 = 1;
	}
	break;

	case 3: // North-East
		break;

	case 4: // East
	{
		dst = m_data + m_width + (m_width-1);
		src = hmap.m_data + hmap.m_width + 1;
		pitch1 = m_width;
		pitch2 = hmap.m_width;
	}
	break;

	case 5: // South-East
		break;

	case 6: // South
	{
		dst = m_data + (m_height - 1)*m_width + 1;
		src = hmap.m_data + hmap.m_width + 1;
		pitch1 = 1;
		pitch2 = 1;
	}
	break;

	case 7: // South-West
		break;

	default:
		assert(0);
		break;
	}

	if (src)
	{
		for (int i = 0; i < m_width-2; ++i)
		{
			const float v2 = *src;
			*dst = v2;
			dst += pitch1;
			src += pitch2;
		}
	}
}


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
void cHeightmap::EdgeSmooth(const cHeightmap *adjacent[8])
{
	for (int i=0; i < 8; ++i)
		if (adjacent[i])
			EdgeSmooth(*adjacent[i], i);
}


// outer memory settng
void cHeightmap::EdgeSmoothBySelf()
{
	//const float minDepth = -10.f;
	//for (int i = 0; i < src.m_width; ++i)
	//	m_data[i] = minDepth;
	//for (int i = 0; i < src.m_width; ++i)
	//	m_data[(src.m_height - 1)*src.m_width + i] = minDepth;
	//for (int i = 0; i < src.m_height; ++i)
	//	m_data[i*src.m_width] = minDepth;
	//for (int i = 0; i < src.m_height; ++i)
	//	m_data[i*src.m_width + src.m_width - 1] = minDepth;

	const float minDepth = -10.f;
	const float offset = -0.1001f;

	for (int i = 0; i < m_width; ++i)
		m_data[i] = m_data[i + m_width] + offset;
	for (int i = 0; i < m_width; ++i)
		m_data[(m_height - 1)*m_width + i] = m_data[(m_height - 2)*m_width + i] + offset;
	for (int i = 0; i < m_height; ++i)
		m_data[i*m_width] = m_data[i*m_width + 1] + offset;
	for (int i = 0; i < m_height; ++i)
		m_data[i*m_width + m_width - 1] = m_data[i*m_width + m_width - 2] + offset;
}


// adjacent : W - NW - N - NE - E - SE - S - SW
void cHeightmap::SmoothVtx(cHeightmap adjacent[8])
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
void cHeightmap::Resize(const int size)
{
	if (size != 258)
		return;

	const int nw = size;
	const int nh = size;
	//float *temp = new float[nw * nh];
	float *temp = (float*)g_memPool258.Alloc();
	assert(temp);
	ZeroMemory(temp, sizeof(float) * nw * nh);
	const float scale = (float)(m_width-2) / (float)(size-2); // ignore outer memory
	
	for (int y = 0; y < nh-2; ++y) // ignore outer memory
	{
		for (int x = 0; x < nw-2; ++x) // ignore outer memory
		{
			const int srcIdx = (int)(y*scale) * m_width + m_width + (int)(x*scale) + 1; // ignore outer memory
			temp[(y+1)*nw + x + 1] = m_data[srcIdx];
		}
	}

	//delete[] m_data; 
	g_memPool67.Free(m_data);
	m_data = temp;
	m_width = nw;
	m_height = nh;

	EdgeSmoothBySelf();
}


// 높이맵 3X3 가우시안 스무딩
// http://iskim3068.tistory.com/41
void cHeightmap::GaussianSmoothing()
{
	const float MaskGaussian[3][3] = { 
		 { 1, 2, 1 }
		,{ 2, 4, 2 }
		,{ 1, 2, 1 } 
	};

	float *p = (float*)g_memPool258.Alloc();
	assert(p);
	const int h = m_height - 1;
	const int w = m_width - 1;
	//for (int i = 1; i < h; i++)
	for (int i = 2; i < h-3; i++)
	{
		//for (int k = 1; k < w; k++)
		for (int k = 2; k < w-3; k++)
		{
			float newValue = 0;
			for (int mr = 0; mr < 3; mr++)
				for (int mc = 0; mc < 3; mc++)
					newValue += MaskGaussian[mr][mc] * *(m_data + ((i + mr - 1) * m_width) + k + mc - 1);

			newValue /= 16.f; // 마스크의 합의 크기로 나누기
			*(p + (i * m_width) + k) = newValue;
		}
	}

	for (int y = 2; y < h-3; y++)
		for (int x = 2; x < w-3; x++)
			*(m_data + (y * m_width) + x) = *(p + (y * m_width) + x);

	g_memPool258.Free(p);
}


StrPath cHeightmap::GetFileName(const char *directoryName
	, const int level, const int xLoc, const int yLoc)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.bil", directoryName, level, yLoc, yLoc, xLoc);
	return path;
}


StrPath cHeightmap::GetFileName(const char *directoryName)
{
	return GetFileName(directoryName, m_level, m_xLoc, m_yLoc);
}


float cHeightmap::GetHeight(const Vector2 &uv)
{
	RETV(!m_data, 0);

	const Vector2 pos(max(0, min(1, uv.x)), max(0, min(1, uv.y)));
	const int x = (int)((m_width - 3) * pos.x);
	const int y = (int)((m_height - 3) * pos.y);
	const float val = m_data[ (m_width * (y + 1)) + x + 1];
	return val;
}


float cHeightmap::GetMaxHeight()
{
	RETV(!m_data, 0);

	float maxH = FLT_MIN;
	const int n = m_width * m_height;
	float *p = m_data;
	for (int i = 0; i < n; ++i, ++p)
		if (maxH < *p)
			maxH = *p;

	const float h = (maxH - 0.1f) * 2500.f;
	return h;
}


// 높이맵을 텍스쳐에 업데이트 한다.
void cHeightmap::UpdateTexture(graphic::cRenderer &renderer)
{
	RET(!m_texture);

	D3D11_MAPPED_SUBRESOURCE desc;
	if (BYTE *dst = (BYTE*)m_texture->Lock(renderer, desc))
	{
		UpdateTexture(desc);
		//float *src = m_data;
		//const int srcPitch = m_width / 2;
		//for (int i = 0; i < m_height; ++i)
		//{
		//	memcpy(dst, src, srcPitch * sizeof(float));
		//	dst += desc.RowPitch;
		//	src += srcPitch;
		//}
		m_texture->Unlock(renderer);
	}
}


// 높이맵을 텍스쳐에 업데이트 한다.
void cHeightmap::UpdateTexture(const D3D11_MAPPED_SUBRESOURCE &desc)
{
	RET(!m_texture);

	BYTE *dst = (BYTE*)desc.pData;
	float *src = m_data;
	const int srcPitch = m_width;
	for (int i = 0; i < m_height; ++i)
	{
		memcpy(dst, src, srcPitch * sizeof(float));
		dst += desc.RowPitch;
		src += srcPitch;
	}
}


void cHeightmap::Clear()
{
	//SAFE_DELETEA(m_data);
	if (m_data)
	{
		g_memPool258.Free(m_data);
		m_data = NULL;
	}

	//m_texture.Clear();
	if (m_texture)
	{
		//g_memPoolTex.Free(m_texture);
		delete m_texture;
		m_texture = NULL;
	}

	m_waitState = 0;
	m_isTextureUpdate = false;
}
