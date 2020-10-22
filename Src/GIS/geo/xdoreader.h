//
// 2018-04-25, jjuiddong
// Real3D Model Data (*.xdo) file reader
//
#pragma once


class cXdoReader : public graphic::iParallelLoadObject
{
public:
	struct sXdo
	{
		DWORD vertexCount;
		graphic::sVertexNormTex *vertices;
		DWORD indexCount;
		u_short *indices;
		DWORD color;
		BYTE imageLevel;
		char imageName[64];
	};

	cXdoReader();
	cXdoReader(graphic::cRenderer &renderer, const char *fileName);
	cXdoReader(graphic::cRenderer &renderer, const cXdoReader *src, const char *fileName
		, const graphic::sFileLoaderArg2 &args) {
		throw std::exception();
	}
	virtual ~cXdoReader();
	virtual const char* Type() override { return "cXdoReader"; }

	bool Read(const char *fileName, const DWORD ver= 0x02000003);
	bool LoadMesh(graphic::cRenderer &renderer);
	void CalcAltitude(const Vector2d &lonLat);
	Quaternion GetRotation(const common::Vector2 &offsetLonLat);
	void Clear();

	static StrPath GetFileName(const StrPath &directoryName
		, const int level, const int xLoc, const int yLoc
		, const char *dataFileName);


protected:
	// ver 3.0.0.1
	bool ReadV1(std::ifstream &ifs);
	// ver 3.0.0.2
	bool ReadV2(std::ifstream &ifs);

	bool Read3DMeshDataBlock(std::ifstream &ifs, OUT sXdo &out);


public:
	BYTE m_type;
	DWORD m_objectId;
	char m_key[64];
	gis::sAABBox m_objBox;
	float m_altitude;
	float m_altitude2;
	BYTE m_faceNum;
	StrPath m_fileName;
	Vector2d m_lonLat;
	vector<sXdo> m_xdos;

	enum LOAD_STATE {READ_MODEL, READ_TEXTURE, FINISH};
	LOAD_STATE m_loadState; // 0:read model, 1:read texture, 2:finish
};
