//
// 2018-05-01, jjuiddong
// Terrain Tile Texture Load
//
#pragma once



class cTileTexture
{
public:
	cTileTexture();
	cTileTexture(graphic::cRenderer &renderer, const char *fileName);
	cTileTexture(graphic::cRenderer &renderer, const cTileTexture *src, const char *fileName
		, const graphic::sFileLoaderArg &args) {
		throw std::exception();
	}
	virtual ~cTileTexture();
	
	bool Create(graphic::cRenderer &renderer, const char *fileName);
	void Clear();

	static StrPath GetFileName(const char *directoryName, const int level, const int xLoc, const int yLoc);
	static StrPath GetFileName(const char *directoryName, const int level, const int xLoc, const int yLoc
		, const char *fileName );


public:
	graphic::cTexture m_tex;
};
