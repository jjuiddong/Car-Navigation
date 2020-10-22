
#include "stdafx.h"
#include "pathfile.h"


cPathFile::cPathFile(const StrPath &fileName //= ""
)
{
	if (!fileName.empty())
		Read(fileName);
}

cPathFile::~cPathFile()
{
	Clear();
}


bool cPathFile::Read(const StrPath &fileName)
{
	cSimpleData simData(fileName.c_str());
	if (!simData.IsLoad())
		return false;

	Clear();

	m_table.reserve(simData.m_table.size());

	Vector2d p0;
	for (auto &row : simData.m_table)
	{
		if (row.size() < 3)
			continue;

		sRow info;
		info.dateTime = common::GetCurrentDateTime6(row[0]);
		info.lonLat.x = atof(row[1].c_str());
		info.lonLat.y = atof(row[2].c_str());

		// 전 좌표와 너무 큰차이가 나면 무시한다. (위경도로 계산)
		if (!p0.IsEmpty() && (p0.Distance(info.lonLat) > 0.08f))
			continue;
		if (!gis::Check6Val(info.lonLat.x))
			continue;
		if (!gis::Check6Val(info.lonLat.y))
			continue;

		p0 = info.lonLat;
		m_table.push_back(info);
	}

	return true;
}


bool cPathFile::IsLoad() const
{
	return !m_table.empty();
}


void cPathFile::Clear()
{
	m_table.clear();
}
