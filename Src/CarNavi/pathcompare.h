//
// 2019-12-24, jjuiddong
// path compare
//
#pragma once


class cPathCompare
{
public:
	struct sPath {
		uint id;
		vector<Vector3> poss;
	};

	// path range
	struct sRange {
		uint s; // start index
		uint e; // end index, if maximum limit uint value, end of path
	};

	struct sEdge {
		uint from; // position index
		uint to; // position index
	};

	// path overlab
	struct sOverlap {
		uint id0; // path0 id
		uint id1; // path1 id
		sRange r0; // path0 range
		sRange r1; // path1 range
		float variance;
	};

	// path segment
	struct sSegment {
		uint id; // path id
		sEdge edge;
	};

	cPathCompare();
	virtual ~cPathCompare();

	bool Compare(const StrPath &pathDirectory);


protected:
	bool IntergratePath(const vector<sPath*> &paths
		, OUT vector<sPath*> &out);
	bool SearchSimilarPath(const uint id
		, const Vector3 &pos
		, const vector<sPath*> &paths
		, OUT vector<sSegment> &out
		);


public:
};
