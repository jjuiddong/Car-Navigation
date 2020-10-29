
#include "stdafx.h"
#include "pathlist.h"

using namespace optimize;


cPathList::cPathList(cOptimizeHistoryFile &history, const list<string> &files)
	: m_history(history)
	, m_line(0)
	, m_fileIdx(0)
	, m_progress(0)
{
	for (auto &fileName : files)
	{
		if (history.IsComplete(fileName))
			continue;
		m_files.push_back(fileName);
	}
	CalcProgress();
}

cPathList::~cPathList()
{
	Clear();
}


// is end of file
bool cPathList::IsEnd()
{
	return (m_files.size() <= m_fileIdx)
		&& (m_pathFile.m_table.size() <= m_line);
}


// get next pos
Vector3 cPathList::GetNextPos()
{
	if (!m_pathFile.IsLoad() || (m_pathFile.m_table.size() <= m_line))
	{
		m_pathFile.Clear();

		while (m_files.size() > m_fileIdx)
		{
			// if exist *.3dpath file, read this file. (binary format)
			StrPath fileName = m_files[m_fileIdx++];
			const StrPath binPathFileName = fileName.GetFileNameExceptExt2() + ".3dpath";
			if (!binPathFileName.IsFileExist())
				continue; // error occurred!!
			if (!m_pathFile.Read(binPathFileName))
				continue; // error occurred!!
			break; // read success
		}

		if (!m_pathFile.IsLoad())
			return Vector3(); // end of file

		m_line = 0;
	}

	const Vector3 pos = m_pathFile.m_table[m_line++].pos;

	// last data? complete this path file
	if (m_pathFile.m_table.size() <= m_line)
		m_history.AddHistory(m_files[m_fileIdx - 1], true, 0);

	CalcProgress();

	m_prevPos = m_curPos;
	m_curPos = pos;
	return pos;
}


// return p0-p1 path
std::pair<Vector3, Vector3> cPathList::GetNextPath()
{
	if (IsEnd())
		return { Vector3(), Vector3() };
	Vector3 pos = GetNextPos();
	if (m_prevPos == Vector3::Zeroes)
	{
		if (IsEnd())
			return { Vector3(), Vector3() };
		pos = GetNextPos();
	}
	return { m_prevPos, pos };
}


float cPathList::CalcProgress()
{
	if (m_files.empty())
	{
		m_progress = 1.0f;
		return 1.f;
	}

	const int idx = std::max(0, (int)(m_fileIdx - 1));
	const float r0 = (float)idx / (float)m_files.size();
	const float r1 = ((float)m_line / (float)m_pathFile.m_table.size())
		* (1.f / (float)m_files.size());
	m_progress = r0 + r1;
	return m_progress;
}


void cPathList::Clear()
{
	m_files.clear();
}
