
#include "stdafx.h"
#include "poireader.h"


cPoiReader::cPoiReader()
{
}

cPoiReader::cPoiReader(graphic::cRenderer &renderer, const char *fileName)
{
	//if (!Read(fileName))
	//	throw std::exception();
	Read(fileName);
}

cPoiReader::cPoiReader(graphic::cRenderer &renderer, const cPoiReader *src, const char *fileName
	, const graphic::sFileLoaderArg2 &args)
{
	throw std::exception();
}

cPoiReader::~cPoiReader()
{
	Clear();
}


bool cPoiReader::Read(const char *fileName)
{
	using namespace std;

	Clear();

	ifstream ifs(fileName, ios::binary);
	if (!ifs.is_open())
		return false;

	m_fileName = fileName;

	DWORD ver;
	ifs.read((char*)&ver, sizeof(ver));
	if ((ver != 0x02000000)
		&& (ver != 0x02000001)
		&& (ver != 0x02000002)
		&& (ver != 0x02000003) // current
		)
	{
		//remove(fileName); // file remove
		return false;
	}

	int poiCount = 0;
	ifs.read((char*)&poiCount, sizeof(poiCount));

	for (int i = 0; i < poiCount; ++i)
	{
		sPoi poi;
		if (!ReadPoi(ifs, poi))
			break;
		m_pois.push_back(poi);
	}

	return true;
}


bool cPoiReader::ReadPoi(std::ifstream &ifs, OUT sPoi &out)
{
	ZeroMemory(&out, sizeof(out));

	ifs.read((char*)&out.lonLat, sizeof(out.lonLat));
	ifs.read((char*)&out.altitude, sizeof(out.altitude));

	u_short keyLen = 0;
	ifs.read((char*)&keyLen, sizeof(keyLen));
	ifs.read(out.key, min(sizeof(out.key) - 1, (uint)keyLen));

	u_short textLen = 0;
	ifs.read((char*)&textLen, sizeof(textLen));
	Str64 text;
	ifs.read(text.m_str, min(text.SIZE - 1, (uint)textLen));
	out.text = text.wstr();

	u_short fontLen = 0;
	ifs.read((char*)&fontLen, sizeof(fontLen));
	ifs.read(out.fontName, min(sizeof(out.fontName) - 1, (uint)fontLen));

	ifs.read((char*)&out.fontSize, sizeof(out.fontSize));
	ifs.read((char*)&out.fontWeight, sizeof(out.fontWeight));
	ifs.read((char*)&out.fontColor, sizeof(out.fontColor));
	ifs.read((char*)&out.fontColorOuter, sizeof(out.fontColorOuter));
	ifs.read((char*)&out.imgKey, sizeof(out.imgKey));

	out.relPos = Vector3(0, 0, 0); // empty vector

	return true;
}


void cPoiReader::Clear()
{
	m_pois.clear();
}


StrPath cPoiReader::GetFileName(const StrPath &directoryName, const int level, const int xLoc, const int yLoc
	, gis::eLayerName::Enum layerName)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d", directoryName.c_str()
		, level, yLoc, yLoc, xLoc);

	switch (layerName)
	{
	case gis::eLayerName::POI_BASE: path += ".poi_base"; break;
	case gis::eLayerName::POI_BOUND: path += ".poi_bound"; break;
	default: assert(0); break;
	}

	return path;
}
