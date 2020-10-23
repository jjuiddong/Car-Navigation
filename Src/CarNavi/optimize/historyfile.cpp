
#include "stdafx.h"
#include "historyfile.h"

using namespace optimize;

cOptimizeHistoryFile::cOptimizeHistoryFile(const StrPath &fileName //= ""
)
{
	if (!fileName.empty())
		Read(fileName);
}

cOptimizeHistoryFile::~cOptimizeHistoryFile()
{
}


// read optimize history file
bool cOptimizeHistoryFile::Read(const StrPath &fileName)
{
	cSimpleData simData(fileName.c_str());
	if (!simData.IsLoad())
		return false;

	Clear();

	m_table.reserve(simData.m_table.size());

	Vector2d p0;
	for (auto &row : simData.m_table)
	{
		if (row.size() < 2)
			continue; // error occurred!!

		sRow info;
		info.fileName = row[0];
		info.isComplete = row[1] == "complete";
		if (!info.isComplete && (row.size() < 3))
			continue; // error occurred!!
		info.line = atoi(row[2].c_str());
		m_table.push_back(info);
	}

	return true;
}


// write history file
bool cOptimizeHistoryFile::Write(const StrPath &fileName)
{
	std::ofstream ofs(fileName.c_str());
	if (!ofs.is_open())
		return false;

	for (auto &row : m_table)
	{
		ofs << row.fileName << ", ";
		ofs << row.isComplete ? "complete, " : "processing, ";
		if (!row.isComplete)
			ofs << row.line;
		ofs << std::endl;
	}
	return true;
}


bool cOptimizeHistoryFile::IsLoad()
{
	return !m_table.empty();
}


void cOptimizeHistoryFile::Clear()
{
	m_table.clear();
}
