
#include "stdafx.h"
#include "binpathfile.h"


cBinPathFile::cBinPathFile(const StrPath &fileName //=""
)
{
	if (!fileName.empty())
		Read(fileName);
}

cBinPathFile::~cBinPathFile()
{
	Clear();
}


// read *.3dpath file
bool cBinPathFile::Read(const StrPath &fileName)
{
	std::ifstream ifs(fileName.c_str(), std::ios::binary);
	if (!ifs.is_open())
		return false;

	Clear();

	uint size = 0;
	ifs.read((char*)&size, sizeof(size));
	if (size <= 0)
		return false;

	m_table.resize(size);
	ifs.read((char*)&m_table[0], sizeof(sRow)*size);
	return true;
}


bool cBinPathFile::IsLoad() const
{
	return !m_table.empty();
}


void cBinPathFile::Clear()
{
	m_table.clear();
}
