
#include "stdafx.h"
#include "heightmap3.h"

using namespace graphic;
using namespace gis2;

const float cHeightmap3::DEFAULT_H = 2.f;


cHeightmap3::cHeightmap3()
	: m_level(0)
	, m_x(0)
	, m_y(0)
	, m_width(0)
	, m_height(0)
	, m_data(nullptr)
	, m_maxH(1.f)
	, m_minH(-1.f)
	, m_texture(nullptr)
	, m_isTextureUpdate(false)
{
}

cHeightmap3::cHeightmap3(graphic::cRenderer &renderer, const char *fileName)
	: m_level(0)
	, m_x(0)
	, m_y(0)
	, m_width(0)
	, m_height(0)
	, m_data(nullptr)
	, m_texture(nullptr)
	, m_isTextureUpdate(false)
{
	if (!ReadAndCreateTexture(renderer, fileName))
		throw std::exception();
}

cHeightmap3::cHeightmap3(graphic::cRenderer &renderer, const cHeightmap3 *src,
	const char *fileName, const sHeightmapArgs2 &args)
{
	if (!Create(renderer, *src, args.huvs, args.level, args.x, args.y))
		throw std::exception();
	m_fileName = fileName;
}

cHeightmap3::~cHeightmap3()
{
	Clear();
}


// copy src heightmap
// copy src from huvs[4]
bool cHeightmap3::Create(cRenderer &renderer, const cHeightmap3 &src
	, const float huvs[4]
	, const int level, const int x, const int y)
{
	// nothing~
	return false;
}


// read heightmap custom file format
// file format
//	- file type: 'dem' 3byte
//	- width: unsigned short 4byte
//	- height: unsigned short 4byte
//	- heightmap: width * height * float(4byte)
bool cHeightmap3::ReadCustom(const char *fileName)
{
	Clear();

	std::ifstream ifs(fileName, std::ios::binary);
	if (!ifs.is_open())
		return false;

	char fmt[4] = { 0, };
	ifs.read(fmt, 3);
	if (string("dem") != fmt)
		return false;

	ifs.read((char*)&m_width, sizeof(m_width));
	ifs.read((char*)&m_height, sizeof(m_height));
	if (m_width * m_height > 10000000)
		return false; // too large
	m_data = new float[m_width * m_height];
	ifs.read((char*)m_data, sizeof(float) * m_width * m_height);

	UpdateLocationByString(fileName);
	return true;
}


// read heightmap file where directoryName, level, x, y
bool cHeightmap3::Read(const char *directoryName, 
	const int level, const int x, const int y)
{
	const int ty = (1 << level) - y - 1;
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.bil", directoryName, level, ty, ty, x);
	return Read(path.c_str());
}


// read ArcGis heightmap file (lerc)
// parse level, x, y by filename
// filename format: *//level//yloc//yloc_xloc.bil
bool cHeightmap3::Read(const char *fileName)
{
	using namespace std;

	Clear();

	cCntZImg czimg;
	float *buffer = czimg.UnCompress(fileName);
	if (!buffer)
		return false;

	m_width = czimg.m_width;
	m_height = czimg.m_height;
	m_data = buffer;
	return true;
}


// write heightmap data custom format
// file format
//	- file type: 'dem' 3byte
//	- width: unsigned short 4byte
//	- height: unsigned short 4byte
//	- heightmap: width * height * float(4byte)
bool cHeightmap3::Write(const StrPath &directoryName)
{
	if (!m_data)
		return false;

	const StrPath fileName = GetFileName(directoryName.c_str());
	std::ofstream ofs(fileName.c_str(), std::ios::binary);
	if (!ofs.is_open())
		return false;

	ofs.write("dem", 3);
	ofs.write((char*)&m_width, sizeof(m_width));
	ofs.write((char*)&m_height, sizeof(m_height));
	ofs.write((char*)m_data, m_width * m_height * sizeof(float));

	return true;
}


// read heightmap and then create texture buffer
bool cHeightmap3::ReadAndCreateTexture(graphic::cRenderer &renderer, 
	const char *fileName)
{
	if (!Read(fileName))
		return false;

	if (m_texture = new graphic::cTexture())
	{
		m_texture->Create(renderer, m_width, m_height
			, DXGI_FORMAT_R32_FLOAT
			, m_data, m_width * sizeof(float)
			, D3D11_USAGE_DYNAMIC);

		m_isTextureUpdate = true;
	}

	return m_texture ? true : false;
}


StrPath cHeightmap3::GetFileName(const StrPath &directoryName
	, const int level, const int x, const int y)
{
	const int ty = (1 << level) - y - 1;
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.bil", directoryName.c_str(), level, ty, ty, x);
	return path;
}


StrPath cHeightmap3::GetFileName(const char *directoryName)
{
	return GetFileName(directoryName, m_level, m_x, m_y);
}


float cHeightmap3::GetHeight(const Vector2 &uv)
{
	RETV(!m_data, 0);

	const Vector2 pos(max(0.f, min(1.f, uv.x)), max(0.f, min(1.f, uv.y)));
	const int x = (int)((m_width - 1) * pos.x);
	const int y = (int)((m_height - 1) * pos.y);
	const float val = m_data[(m_width * y) + x];
	return val * 0.05f;
}


// clac heightmap min/max boundingbox
bool cHeightmap3::CalcBoundingBox()
{
	RETV(!m_data, false);

	float maxH = FLT_MIN;
	float minH = FLT_MAX;
	const int n = m_width * m_height;
	float *p = m_data;
	for (int i = 0; i < n; ++i, ++p)
	{
		if (maxH < *p)
			maxH = *p;
		if (minH > *p)
			minH = *p;
	}

	m_maxH = std::max(DEFAULT_H, maxH * 0.05f);
	m_minH = minH * 0.05f;
	return true;
}


// update level, x, y attribute by string
// string: maybe filename, ex) *//level//y//y_x.bil
bool cHeightmap3::UpdateLocationByString(const string &str)
{
	vector<string> out;
	common::tokenizer(str, "\\", {}, out);
	if (out.size() < 4)
		return false;
	
	const int s = out.size();
	const int level = atoi(out[s - 3].c_str());
	int y = 0, x = 0;
	const int cnt = sscanf(out[s - 1].c_str(), "%d_%d.bill", &y, &x);
	RETV(cnt < 2, false);
	m_level = level;
	m_x = x;
	m_y = (1 << level) - y - 1; // change arcgis space -> original space
	return true;
}


void cHeightmap3::Clear()
{
	SAFE_DELETEA(m_data);
	SAFE_DELETE(m_texture);
	m_isTextureUpdate = false;
}
