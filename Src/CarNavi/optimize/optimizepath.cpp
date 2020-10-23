
#include "stdafx.h"
#include "optimizepath.h"
#include "historyfile.h"

using namespace optimize;


cOptimizePath::cOptimizePath()
	: m_isLoop(true)
	, m_pointMapper(nullptr)
{
}

cOptimizePath::~cOptimizePath()
{
	Clear();
}


// optimize path
bool cOptimizePath::Optimize(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain)
{
	if (!m_pointMapper)
		m_pointMapper = new cPointMapper();

	Cancel();

	// check to process optimize path file
	cOptimizeHistoryFile history("optimize_history.txt");

	list<string> files;
	list<string> exts;
	exts.push_back(".txt");
	common::CollectFiles(exts, "path", files);

	for (auto &fileName : files)
	{
		// if exist *.3dpath file, read this file. (binary format)
		const StrPath binPathFileName = StrPath(fileName).GetFileNameExceptExt2() 
			+ ".3dpath";
		if (!binPathFileName.IsFileExist())
		{
			// if not exist *.3dpath file, generate file
			cPathFile pathFile(fileName);
			if (!pathFile.IsLoad())
				continue; // error occurred!!
			pathFile.Write3DPathFile(renderer, terrain, binPathFileName);
		}

		if (!binPathFileName.IsFileExist())
			continue; // error occurred!!

		// read binary format path file (*.3dpath)
		cBinPathFile pathFile(binPathFileName);
		if (!pathFile.IsLoad())
			continue; // error occurred!!

		for (auto &row : pathFile.m_table)
		{

			m_pointMapper->addPoint(row.pos);
		}

	}


	//m_isLoop = true;
	//m_thread = std::thread(cOptimizePath::ThreadProc, this);
	return true;
}


// cancel job
bool cOptimizePath::Cancel()
{
	m_isLoop = false;
	if (m_thread.joinable())
		m_thread.join();
	return true;
}


// clear optimize path class
void cOptimizePath::Clear()
{
	SAFE_DELETE(m_pointMapper);
}


// thread procedure
int cOptimizePath::ThreadProc(cOptimizePath *optimizePath)
{
	while (optimizePath->m_isLoop)
	{


		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10ms);
	}

	return 1;
}

