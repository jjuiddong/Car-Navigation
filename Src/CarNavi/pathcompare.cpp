
#include "stdafx.h"
#include "pathcompare.h"


cPathCompare::cPathCompare()
{
}

cPathCompare::~cPathCompare()
{
}


// read all path
// compare each path, intergrate same path
bool cPathCompare::Compare(const StrPath &pathDirectory)
{
	struct sChunk {
		vector<Vector3> path;
		vector<vector<Vector3>> chunks;
	};
	
	list<string> files;
	list<string> exts;
	exts.push_back(".3dpos");
	common::CollectFiles(exts, pathDirectory.c_str(), files);

	// 100 point chunk
	vector<sChunk*> spath; // same path

	// compare same path
	for (auto &fileName : files)
	{
		std::ifstream ifs(fileName.c_str());
		if (!ifs.is_open())
			continue;

		uint size = 0;
		ifs.read((char*)&size, sizeof(size));
		if (size <= 0)
			continue;

		vector<Vector3> points;
		points.resize(size);
		ifs.read((char*)&points[0], sizeof(Vector3)*size);

		if (points.size() < 100)
			continue;

		// read 100 point path
		vector<vector<Vector3>> chunks;
		int cnt = 0;
		vector<Vector3> chunk;
		chunk.reserve(100);
		for (uint i = 0; i < points.size(); ++i)
		{
			if (cnt < 100)
			{
				++cnt;
				chunk.push_back(points[i]);
			}
			else
			{
				chunks.push_back(chunk);
				chunk.clear();
			}
		}

		// same path에서 같은 path가 있는지 검사
		sChunk *same = nullptr;
		
		for (sChunk *c : spath)
		{
			if (c->chunks.empty())
				continue;

			// 대표 path와 검사한다.
			vector<Vector3> &cmp = c->chunks[0];		



		}

	}

	return true;
}
