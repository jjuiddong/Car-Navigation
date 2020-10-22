//
// 2018-04-25, jjuiddong
// Point of Interest (*.poi) File reader
//
#pragma once


class cPoiReader : public graphic::iParallelLoadObject
{
public:
	struct sPoi
	{
		Vector2d lonLat; // longitude, latitude
		Vector3 relPos; // setting internal process
		float altitude;
		char key[64];
		WStr64 text;
		char fontName[64];
		BYTE fontSize;
		BYTE fontWeight;
		DWORD fontColor;		// A8R8G8B8
		DWORD fontColorOuter;	// A8R8G8B8
		char imgKey[10];
	};

	cPoiReader();
	cPoiReader(graphic::cRenderer &renderer, const char *fileName);
	cPoiReader(graphic::cRenderer &renderer, const cPoiReader *src, const char *fileName
		, const graphic::sFileLoaderArg2 &args);

	virtual ~cPoiReader();
	virtual const char* Type() override { return "cPoiReader"; }

	bool Read(const char *fileName);
	void Clear();

	static StrPath GetFileName(const StrPath &directoryName, const int level, const int xLoc, const int yLoc
		, gis::eLayerName::Enum layerName);

	
protected:
	bool ReadPoi(std::ifstream &ifs, OUT sPoi &out);


public:
	StrPath m_fileName;
	vector<sPoi> m_pois;
};
