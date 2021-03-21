
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

		// �� ��ǥ�� �ʹ� ū���̰� ���� �����Ѵ�. (���浵�� ���)
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


// write *.3dpath file, binary format, read from cBinPathFile class
bool cPathFile::Write3DPathFile(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain
	, const StrPath &fileName) const
{
	if (!IsLoad())
		return false; // empty data
	std::ofstream ofs(fileName.c_str(), std::ios::binary);
	if (!ofs.is_open())
		return false;
	const uint size = m_table.size();
	ofs.write((char*)&size, sizeof(size));

	for (auto &row : m_table)
	{
		const Vector3 pos = terrain.Get3DPosPrecise(renderer, row.lonLat);
		ofs.write((char*)&row.dateTime, sizeof(uint64));
		ofs.write((char*)&pos, sizeof(pos));
		const int dummy = 0;
		ofs.write((char*)&dummy, sizeof(int)); // 8byte alignment
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
