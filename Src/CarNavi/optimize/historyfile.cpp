
#include "stdafx.h"
#include "historyfile.h"

using namespace optimize;


cOptimizeHistoryFile::cOptimizeHistoryFile(
	const string &fileName //= ""
)
{
	if (!fileName.empty())
		Read(fileName);
}

cOptimizeHistoryFile::~cOptimizeHistoryFile()
{
	Clear();
}


// read optimize history file
bool cOptimizeHistoryFile::Read(const string &fileName)
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
		info.fileName = common::trim(row[0]);
		info.isComplete = common::trim(row[1]) == "complete";
		if (!info.isComplete && (row.size() < 3))
			continue; // error occurred!!
		info.line = atoi(row[2].c_str());
		m_table.push_back(info);
	}

	return true;
}


// write history file
bool cOptimizeHistoryFile::Write(const string &fileName)
{
	std::ofstream ofs(fileName.c_str());
	if (!ofs.is_open())
		return false;

	for (auto &row : m_table)
	{
		ofs << row.fileName << ", ";
		ofs << (row.isComplete ? "complete, " : "processing, ");
		ofs << (row.isComplete? 0 : row.line);
		ofs << std::endl;
	}
	return true;
}


// add history
bool cOptimizeHistoryFile::AddHistory(const string &fileName
	, const bool isComplete, const int line)
{
	sRow row;
	row.fileName = fileName;
	row.isComplete = isComplete;
	row.line = line;
	m_table.push_back(row);
	return true;
}


// is complete optimize work?
bool cOptimizeHistoryFile::IsComplete(const string &fileName)
{
	for (auto &row : m_table)
		if (fileName == row.fileName)
			return row.isComplete;
	return false;
}


bool cOptimizeHistoryFile::IsLoad()
{
	return !m_table.empty();
}


void cOptimizeHistoryFile::Clear()
{
	m_table.clear();
}
