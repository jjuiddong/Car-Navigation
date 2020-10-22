//
// 2018-05-01, jjuiddong
// Terrain Tile Texture Load
//
#pragma once



class cTileTexture : public graphic::iParallelLoadObject
{
public:
	cTileTexture();
	cTileTexture(graphic::cRenderer &renderer, const char *fileName);
	cTileTexture(graphic::cRenderer &renderer, const cTileTexture *src, const char *fileName
		, const sHeightmapArgs2 &args) {
		throw std::exception(); // nothing~
	}
	virtual ~cTileTexture();
	virtual const char* Type() override { return "cTileTexture"; }
	
	bool Create(graphic::cRenderer &renderer, const char *fileName);
	void Clear();

	static StrPath GetFileName(const StrPath &directoryName, const int level, const int xLoc, const int yLoc);
	static StrPath GetFileName(const StrPath &directoryName, const int level, const int xLoc, const int yLoc
		, const char *fileName );


public:
	graphic::cTexture m_tex;
};
