
#include "stdafx.h"
#include "landmark.h"


cLandMark::cLandMark()
{
}

cLandMark::~cLandMark()
{
	Clear();
}


// read landmark file
bool cLandMark::Read(const StrPath &fileName)
{
	cSimpleData simData(fileName.c_str());
	if (!simData.IsLoad())
		return false;

	for (auto &row : simData.m_table)
	{
		if (row.size() < 3)
			continue;

		sLandMark landMark;
		landMark.name = row[0];
		landMark.lonLat.x = atof(row[1].c_str());
		landMark.lonLat.y = atof(row[2].c_str());
		m_landMarks.push_back(landMark);
	}

	return true;
}


// write landmark data
bool cLandMark::Write(const StrPath &fileName)
{
	cSimpleData simData;
	for (auto &landMark : m_landMarks)
	{
		vector<string> row(3);
		row[0] = landMark.name.c_str();
		row[1] = common::format("%.15f", landMark.lonLat.x);
		row[2] = common::format("%.15f", landMark.lonLat.y);
		simData.m_table.push_back(row);
	}

	if (!simData.Write(fileName.c_str()))
		return false;

	return true;
}


bool cLandMark::AddLandMark(const StrId &name, const Vector2d &lonLat)
{
	sLandMark landMark;
	landMark.name = name;
	landMark.lonLat = lonLat;
	m_landMarks.push_back(landMark);
	return true;
}


void cLandMark::Clear()
{
	m_landMarks.clear();
}
