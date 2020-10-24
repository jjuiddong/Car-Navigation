//
// 2020-10-22, jjuiddong
// point mapper
//	- path point management
//	- quadtree manage
//		- quad tree level 10 ~ 15 
//	- fast access point
//	- fast compare point
//
#pragma once

namespace optimize
{
	// 3D position table, unique pos management
	struct sMappingPos
	{
		int cnt;
		Vector3 pos;
	};

	struct sMapping
	{
		vector<sMappingPos> table;
		vector<int> indices; // lv10 table pointed
	};


	// cPointMapper
	class cPointMapper
	{
	public:
		cPointMapper();
		virtual ~cPointMapper();

		bool Read(const StrPath &fileName);
		bool Write(const StrPath &fileName);
		Vector3 addPoint(const Vector3 &pos);
		void Clear();


	protected:
		cQuadTree<sMapping>* findQuadTree(int lv, int xLoc, int yLoc);
		Vector3 addPointInBestNode(cQuadTree<sMapping> *qtree
			, const sRectf &rect, const Vector3 &pos);


	public:
		enum { TREE_LEVEL = 10, MAX_TABLESIZE = 200, };
		map<uint64, cQuadTree<sMapping>*> m_qtrees; //key: lv + xLoc + yLoc
	};
}
