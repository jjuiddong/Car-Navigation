//
// 2020-10-24, jjuiddong
// QuadTree + Graph
//	- QuadTree Node
//		- point table
//		- graph 
//
#pragma once


namespace optimize
{
	// quadtree graph id
	// idx + lv + xLoc + yLoc
	// idx(26bit) + lv(4bit) + xLoc(17bit) + yLoc(17bit)
	typedef unsigned __int64 qgid;

	// parse cQTreeGraph id -> index, level, xloc, yloc
#define PARSE_QGID(id, index, level, xloc, yloc) \
		{\
			const int maxLv = cQuadTree<>::MAX_LEVEL; \
			index = id >> (maxLv + maxLv + 8 + 4); \
			level = (id >> (maxLv + maxLv + 8)) & 0xF; \
			yloc = (id >> (maxLv + 4)) & 0xFFFFF; \
			xloc = id & 0xFFFFF; \
		}

	// parse cQTreeGraph id -> index
#define PARSE_QGID_INDEX(id, index) \
		{\
			const int maxLv = cQuadTree<>::MAX_LEVEL; \
			index = id >> (maxLv + maxLv + 8 + 4); \
		}

	// make cQTreeGraph id from table index, level, xloc, yloc
#define MAKE_QGID(id, index, level, xloc, yloc) \
		{\
			const int maxLv = cQuadTree<>::MAX_LEVEL; \
			id = (((uint64)(index)) << (maxLv + maxLv + 8 + 4)) \
			 + (((uint64)(level)) << (maxLv + maxLv + 8)) \
			 + (((uint64)(yloc)) << (maxLv + 4)) \
			 + ((uint64)(xloc)); \
		}

	// compare quadtree id (lv + xLoc + yLoc)
#define COMPARE_QID(id0, id1) \
		(((id0) & 0xFFFFFFFFFFF) == ((id1) & 0xFFFFFFFFFFF))


	struct sEdge
	{
		qgid from;
		qgid to;
		sEdge() : from(0), to(0) {}
		sEdge(qgid _from, qgid _to) :from(_from), to(_to) {}
	};
	// to store container set<sEdge> 
	static bool operator == (const sEdge &lhs, const sEdge &rhs) {
		return ((lhs.from == rhs.from) && (lhs.to == rhs.to))
			|| ((lhs.to == rhs.from) && (lhs.from == rhs.to));
	}
	static bool operator != (const sEdge &lhs, const sEdge &rhs) {
		return !operator==(lhs, rhs);
	}
	static bool operator < (const sEdge &lhs, const sEdge &rhs) {
		if (lhs.from == rhs.from)
			return lhs.to < rhs.to;
		return lhs.from < rhs.from;
	}

	struct sTransition
	{
		qgid to; // connect vertex id
	};

	struct sVertex
	{
		Vector3 pos;
		uint accCnt; // accumlate count
		uint trCnt; // transition count
		uint trCapa; // trs capacity
		sTransition *trs;
		enum {DEFAULT_TRANSITION = 2};
		sVertex() : accCnt(0), trCnt(0), trCapa(0), trs(nullptr) {}
	};

	struct sNode
	{
		vector<sVertex> vertices; // graph vertex
		graphic::cDbgLineList *lineList;
		sNode() : lineList(nullptr) {}
	};


	// QuadTree Graph
	class cQTreeGraph
	{
	public:
		cQTreeGraph();
		virtual ~cQTreeGraph();

		bool ReadFile();
		bool WriteFile();

		qgid AddPoint(const Vector3 &pos, map<qgid, qgid> &mapping
			, const bool isAverage = true);
		
		bool AddTransition(const qgid id0, const qgid id1);
		
		sEdge FindNearEdge(const Vector3 &pos, const float distance
			, const qgid exceptId = 0);

		bool MergePath(cPathList &pathList, const float distance);
		
		Vector3 GetVertexPos(const qgid id);
		
		sQuadTreeNode<sNode>* FindNode(const qgid id);
		sQuadTreeNode<sNode>* FindNode(const int level
			, const int xLoc, const int yLoc);
		sQuadTreeNode<sNode>* FindBestNode(const Vector3 &pos);

		sVertex* FindVertex(const qgid id);
		sVertex* FindVertex(sQuadTreeNode<sNode>*node, const qgid id);
		sTransition* FindTransition(sVertex *vtx, const qgid toId);

		bool AddVertexTransition(sVertex *vtx, const sTransition &tr);

		bool CreateGraphLines(graphic::cRenderer &renderer
			, sQuadTreeNode<sNode> *node);

		void Clear();


	protected:
		cQuadTree<sNode>* FindAndCreateRootTree(const int level
			, const int xLoc, const int yLoc);			
		cQuadTree<sNode>* FindRootTree(const int level
			, const int xLoc, const int yLoc);
		sQuadTreeNode<sNode>* FindBestNode(const int level
			, const int xLoc, const int yLoc);
		sQuadTreeNode<sNode>* FindAndCreateFromFile(const int level
			, const int xLoc, const int yLoc);

		qgid AddPointInBestNode(cQuadTree<sNode> *qtree
			, const sRectf &rect, const Vector3 &pos
			, map<qgid, qgid> &mapping
			, const bool isAverage = true);

		bool DivideNodeToChild(cQuadTree<sNode> *qtree
			, sQuadTreeNode<sNode> *node
			, map<qgid, qgid> &mapping);

		qgid MovePoint(sQuadTreeNode<sNode> *fromNode, const uint index
			, map<qgid, qgid> &mapping
			, const bool isCalcAverage = false);

		bool CalcVertexLayout(sQuadTreeNode<sNode> *node
			, map<qgid, qgid> &mapping);

		bool ReadNode(sQuadTreeNode<sNode> *node);

		bool WriteNode(sQuadTreeNode<sNode> *node);

		StrPath GetNodeFilePath(const int level, const int xLoc, const int yLoc);
		void InitVertex(sVertex &vtx, const Vector3 &pos, const uint accCnt);


	public:
		enum { DEFAULT_TREE_LEVEL = 6, MAX_TABLESIZE = 200, };
		const StrPath s_dir = ".\\path\\optimize";

		map<uint64, cQuadTree<sNode>*> m_roots; //key: lv + xLoc + yLoc
		bool m_isDivide; // divided?
	};

}
