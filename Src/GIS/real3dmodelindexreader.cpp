
#include "stdafx.h"
#include "real3dmodelindexreader.h"


cReal3DModelIndexReader::cReal3DModelIndexReader()
{
}

cReal3DModelIndexReader::cReal3DModelIndexReader(graphic::cRenderer &renderer, const char *fileName)
{
	Read(fileName);
}

cReal3DModelIndexReader::~cReal3DModelIndexReader()
{
	Clear();
}


bool cReal3DModelIndexReader::Read(const char *fileName)
{
	using namespace std;

	Clear();

	ifstream ifs(fileName, ios::binary);
	if (!ifs.is_open())
		return false;

	m_fileName = fileName;

	ifs.read((char*)&m_level, sizeof(m_level));
	ifs.read((char*)&m_xLoc, sizeof(m_xLoc));
	ifs.read((char*)&m_yLoc, sizeof(m_yLoc));
	ifs.read((char*)&m_objectCount, sizeof(m_objectCount));

	if ((u_int)m_level >= cQuadTree<sQuadTreeNode<sQuadData>>::MAX_LEVEL)
		return false;

	for (int i = 0; i < m_objectCount; ++i)
	{
		sObject obj;
		if (!ReadModelObject(ifs, obj))
			break;
		m_objs.push_back(obj);
	}

	return true;
}


bool cReal3DModelIndexReader::ReadModelObject(std::ifstream &ifs, OUT sObject &out)
{
	ZeroMemory(&out, sizeof(out));

	ifs.read((char*)&out.ver, sizeof(out.ver));
	ifs.read((char*)&out.type, sizeof(out.type));

	BYTE keyLen = 0;
	ifs.read((char*)&keyLen, sizeof(keyLen));
	ifs.read(out.key, min(sizeof(out.key) - 1, keyLen));

	ifs.read((char*)&out.centerPos, sizeof(out.centerPos));
	ifs.read((char*)&out.altitude, sizeof(out.altitude));
	ifs.read((char*)&out.objBox, sizeof(out.objBox));
	ifs.read((char*)&out.imgLevel, sizeof(out.imgLevel));

	BYTE dataFileLen = 0;
	ifs.read((char*)&dataFileLen, sizeof(dataFileLen));
	ifs.read(out.dataFile, min(sizeof(out.dataFile) - 1, dataFileLen));

	BYTE imgFileNameLen = 0;
	ifs.read((char*)&imgFileNameLen, sizeof(imgFileNameLen));
	ifs.read(out.imgFileName, min(sizeof(out.imgFileName) - 1, imgFileNameLen));

	return true;
}


void cReal3DModelIndexReader::Clear()
{
	m_objs.clear();
}


StrPath cReal3DModelIndexReader::GetFileName(const char *directoryName, const int level, const int xLoc, const int yLoc)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%04d_%04d.facility_build", directoryName, level, yLoc, yLoc, xLoc);
	return path;
}
