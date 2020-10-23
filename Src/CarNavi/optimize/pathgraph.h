//
// 2020-10-23, jjuiddong
// optimize path, path graph
//	- graph structure
//
#pragma once

namespace optimize
{

	struct sTransition
	{
		uint64 t; // connect quadtree id
		int idx; // quadtree vertex index
	};

	struct sVertex
	{
		uint64 t; // quadtree id
		int idx; // quadtree vertex index
		vector<sTransition> trs;
	};

	class cPointMapper;
	class cPathGraph
	{
	public:
		cPathGraph();
		virtual ~cPathGraph();

		bool addEdge(cPointMapper *ptMapper
			, cQuadTree<sMapping> *qtree0, const int idx0
			, cQuadTree<sMapping> *qtree1, const int idx1);

		bool removeEdge(cPointMapper *ptMapper
			, cQuadTree<sMapping> *qtree0, const int idx0
			, cQuadTree<sMapping> *qtree1, const int idx1);


	public:
		
	};

}
