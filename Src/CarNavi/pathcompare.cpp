
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
	return true;

	list<string> files;
	list<string> exts;
	exts.push_back(".3dpos");
	common::CollectFiles(exts, pathDirectory.c_str(), files);

	vector<sPath*> paths;
	for (auto &fileName : files)
	{
		std::ifstream ifs(fileName.c_str(), std::ios::binary);
		if (!ifs.is_open())
			continue;

		uint size = 0;
		ifs.read((char*)&size, sizeof(size));
		if (size <= 100)
			continue; // ignore small path

		sPath *path = new sPath;
		path->id = paths.size();
		path->poss.resize(size);
		ifs.read((char*)&path->poss[0], sizeof(Vector3)*size);
		paths.push_back(path);
	}

	vector<sPath*> integrates;
	IntergratePath(paths, integrates);

	// remove
	for (auto &p : paths)
		delete p;
	paths.clear();
	for (auto &p : integrates)
		delete p;
	integrates.clear();
	return true;
}


// search all path, intergrate and generate simple path
bool cPathCompare::IntergratePath(const vector<cPathCompare::sPath*> &paths
	, OUT vector<cPathCompare::sPath*> &out)
{
	vector<cPathCompare::sPath*> all; // intergrate path

	for (uint i = 0; i < paths.size(); ++i)
	{
		sPath *path = paths[i];

		sPath *integrate = nullptr; // new path
		for (uint k = 0; k < path->poss.size(); ++k)
		{
			const Vector3 &pos = path->poss[k];

			vector<cPathCompare::sSegment> segs;
			if (SearchSimilarPath(i, pos, all, segs))
			{
				// insert new point or ignore
				integrate = nullptr;
				continue;
			}

			Vector3 avrPos = pos;
			if (SearchSimilarPath(i, pos, paths, segs))
			{
				Vector3 tmp = pos;
				for (auto &seg : segs)
				{
					const Vector3 &p0 = paths[seg.id]->poss[seg.edge.from];
					const Vector3 &p1 = paths[seg.id]->poss[seg.edge.to];
					common::Line line(p0, p1);
					tmp += line.Projection(pos);
				}
				avrPos = tmp / (float)(segs.size() + 1);
			}

			if (integrate)
			{
				integrate->poss.push_back(avrPos);
			}
			else
			{
				integrate = new sPath;
				integrate->poss.push_back(avrPos);
				all.push_back(integrate);
			}
		}
	}

	out = all;

	return true;
}


// search and collect near path segment
bool cPathCompare::SearchSimilarPath(const uint id
	, const Vector3 &pos
	, const vector<cPathCompare::sPath*> &paths
	, OUT vector<cPathCompare::sSegment> &out
)
{
	// limit distance is 30m
	const float LIMIT = gis::Meter23DUnit(30.f);

	for (uint i=0; i < paths.size(); ++i)
	{
		if (i == id)
			continue;

		sPath *path = paths[i];

		sSegment seg; // minimum distance edge
		seg.id = i;
		seg.edge.from = USHORT_MAX;

		float minLen = FLT_MAX;
		for (uint k = 1; k < path->poss.size(); ++k)
		{
			const Vector3 &p0 = path->poss[k - 1];
			const Vector3 &p1 = path->poss[k];

			common::Line line(p0, p1);
			const float len = line.GetDistance(pos);
			if (LIMIT < len)
				continue;
			if (minLen <= len)
				continue;

			minLen = len;
			seg.edge.from = k - 1;
			seg.edge.to = k;
		}

		if (seg.edge.from == USHORT_MAX)
			continue;

		out.push_back(seg);
	}

	return !out.empty();
}
