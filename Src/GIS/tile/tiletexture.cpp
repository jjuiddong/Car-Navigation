
#include "stdafx.h"
#include "tiletexture.h"


cTileTexture::cTileTexture()
{
}

cTileTexture::cTileTexture(graphic::cRenderer &renderer, const char *fileName)
{
	if (!Create(renderer, fileName))
		throw std::exception();
}

cTileTexture::~cTileTexture()
{
	Clear();
}


bool cTileTexture::Create(graphic::cRenderer &renderer, const char *fileName)
{
	Clear();
	const bool result = m_tex.Create(renderer, fileName);
	return result;
}


StrPath cTileTexture::GetFileName(const StrPath &directoryName
	, const int level, const int xLoc, const int yLoc)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.dds", directoryName.c_str()
		, level, yLoc, yLoc, xLoc);
	return path;
}


StrPath cTileTexture::GetFileName(const StrPath &directoryName
	, const int level, const int xLoc, const int yLoc
	, const char *fileName)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%s", directoryName.c_str(), level, yLoc, fileName);
	return path;
}


// arcgis filename
StrPath cTileTexture::GetFileName2(const StrPath &directoryName
	, const int level, const int xLoc, const int yLoc)
{
	const int y = (1 << level) - yLoc - 1;
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.dds", directoryName.c_str()
		, level, y, y, xLoc);
	return path;
}


// arcgis filename
StrPath cTileTexture::GetFileName2(const StrPath &directoryName
	, const int level, const int xLoc, const int yLoc
	, const char *fileName)
{
	const int y = (1 << level) - yLoc - 1;
	StrPath path;
	path.Format("%s\\%d\\%04d\\%s", directoryName.c_str(), level, y, fileName);
	return path;
}


void cTileTexture::Clear()
{
	m_tex.Clear();
}
