//
// 2018-04-25, jjuiddong
// Real3D Model Index (*.dat) file reader
//
#pragma once


class cReal3DModelIndexReader : public graphic::iParallelLoadObject
{
public:
	struct sObject
	{
		DWORD ver;
		BYTE type;
		char key[64];
		Vector2d centerPos;
		float altitude;
		gis::sAABBox objBox;
		BYTE imgLevel;
		char dataFile[64];
		char imgFileName[64];
		Vector3 normal; // for debugging
	};

	cReal3DModelIndexReader();
	cReal3DModelIndexReader(graphic::cRenderer &renderer, const char *fileName);
	cReal3DModelIndexReader(graphic::cRenderer &renderer, const cReal3DModelIndexReader *src, const char *fileName
		, const graphic::sFileLoaderArg2 &args) {
		throw std::exception();
	}
	virtual ~cReal3DModelIndexReader();
	virtual const char* Type() override { return "cReal3DModelIndexReader"; }

	bool Read(const char *fileName);
	void Clear();

	static StrPath GetFileName(const StrPath &directoryName, const int level, const int xLoc, const int yLoc);


protected:
	bool ReadModelObject(std::ifstream &ifs, OUT sObject &out);


public:
	int m_level;
	int m_xLoc;
	int m_yLoc;
	int m_objectCount;
	StrPath m_fileName;
	vector<sObject> m_objs;
};
