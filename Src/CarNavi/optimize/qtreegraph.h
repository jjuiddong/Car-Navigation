//
// 2020-10-24, jjuiddong
// QuadTree + Graph
//
#pragma once


namespace optimize
{
	// quadtree graph id
	// idx + lv + xLoc + yLoc
	// idx(26bit) + lv(4bit) + xLoc(17bit) + yLoc(17bit)
	typedef unsigned __int64 qgid;

	struct sEdge
	{
		qgid id0;
		qgid id1;
	};

	struct sTransition
	{
		qgid to; // connect vertex id
	};

	struct sVertex
	{
		qgid id;
		uint trCnt; // transition count
		uint trCapa; // trs capacity
		sTransition *trs;
		enum {DEFAULT_TRANSITION = 2};
		sVertex() : trCnt(0), trCapa(0), trs(nullptr) {}
	};

	// 3D position info
	struct sAccPos
	{
		int cnt; // accumulate count
		Vector3 pos;
	};

	struct sNode
	{
		vector<sAccPos> table; // unique pos management
		vector<sVertex> vertices; // graph vertex
	};


	// QuadTree Graph
	class cQTreeGraph
	{
	public:
		cQTreeGraph();
		virtual ~cQTreeGraph();

		qgid AddPoint(const Vector3 &pos);
		bool AddTransition(const qgid id0, const qgid id1);
		sEdge FindNearEdge(const Vector3 &pos, const float distance);
		void Clear();


	protected:
		cQuadTree<sNode>* FindAndCreateTree(const int lv
			, const int xLoc, const int yLoc);
		cQuadTree<sNode>* FindRootTree(const int level
			, const int xLoc, const int yLoc);
		sQuadTreeNode<sNode>* FindBestNode(const int level
			, const int xLoc, const int yLoc);
		sQuadTreeNode<sNode>* FindNode(const qgid id);
		sVertex* FindVertex(const qgid id);
		sVertex* FindVertex(sQuadTreeNode<sNode>*node, const qgid id);
		sTransition* FindTransition(sVertex *vtx, const qgid toId);

		qgid AddPointInBestNode(cQuadTree<sNode> *qtree
			, const sRectf &rect, const Vector3 &pos);

		bool DivideNodeToChild(cQuadTree<sNode> *qtree
			, sQuadTreeNode<sNode> *node);

		bool AddVertexTransition(sVertex *vtx, const sTransition &tr);

		qgid MakeQgid(const int index
			, const int level, const int xLoc, const int yLoc);
		std::tuple<int,int,int,int> ParseQgid(const qgid id);
		uint64 GetQTreeIdFromQgid(const qgid id);


	public:
		enum { TREE_LEVEL = 10, MAX_TABLESIZE = 200, };
		map<uint64, cQuadTree<sNode>*> m_qtrees; //key: lv + xLoc + yLoc
	};

}
