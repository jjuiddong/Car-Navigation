//
// 2021-03-29, jjuiddong
// Quad-Tree for Geometry (Sparse QuadTree)
//	- upgrade cQuadTree
//
//
#pragma once


namespace gis
{
	
	// maximum quad tree level
	static const int MAX_QTREE_LEVEL = 16;


	// quad tree node
	template<class T>
	struct sQNode : public common::cMemoryPool<sQNode<T>>
	{
		int xLoc;
		int yLoc;
		int level;
		sQNode *parent; // reference
		sQNode *children[4]; // reference
		T data;

		sQNode() {
			xLoc = 0;
			yLoc = 0;
			level = 0;
			parent = nullptr;
			children[0] = children[1] = children[2] = children[3] = nullptr;
		}
	};


	// quad tree
	template<class T>
	class cQuadTree2
	{
	public:
		typedef sQNode<T> qnode;

		cQuadTree2();
		~cQuadTree2();

		bool Insert(qnode *node);
		bool Remove(qnode *node, const bool isRmTree = true);
		bool InsertChildren(qnode *node);
		bool InsertChildren(qnode *node, qnode *child[4]);
		bool RemoveChildren(qnode *node, const bool isRmTree = true);
		qnode* GetNode(const sRectf &rect);
		qnode* GetNode(const int level, const int xLoc, const int yLoc);
		qnode* GetNode(const uint64 key);
		qnode* GetNorthNeighbor(const qnode *node);
		qnode* GetSouthNeighbor(const qnode *node);
		qnode* GetWestNeighbor(const qnode *node);
		qnode* GetEastNeighbor(const qnode *node);

		static std::tuple<int, int, int> GetNodeLevel(const sRectf &rect);
		static std::pair<int, int> GetNodeLocation(const sRectf &rect, const int level);
		static sRectf GetNodeRect(const int level, const int xLoc, const int yLoc);
		static sRectf GetNodeRect(const qnode *node);
		static sRectf GetNodeGlobalRect(const qnode *node);
		static Vector3 GetGlobalPos(const Vector3 &relPos);
		static Vector3 GetRelationPos(const Vector3 &globalPos);
		static uint64 MakeKey(const int level, const int xLoc, const int yLoc);
		void Clear(const bool isRmTree = true);


	public:
		qnode *m_root;
		sRectf m_rect; // root rect
		std::map<uint64, qnode*> m_nodeTable[MAX_QTREE_LEVEL]; // level quad tree map
															  // key = quad node x,y index (linear)
	};


	// m_nodeTable key
	// Level            Key
	//   0               0
	//   1       0       |        1       |       2      |        3
	//   2    0,1,2,3    |    4,5,6,7     |  8,9,10,11   |   12,13,14,15
	//   3    ....

	template<class T>
	inline cQuadTree2<T>::cQuadTree2()
		: m_root(nullptr)
	{
		m_rect = sRectf::Rect(0, 0, 1 << MAX_QTREE_LEVEL, 1 << MAX_QTREE_LEVEL);
	}

	template<class T>
	inline cQuadTree2<T>::~cQuadTree2()
	{
		Clear();
	}


	template<class T>
	inline uint64 cQuadTree2<T>::MakeKey(const int level, const int xLoc, const int yLoc)
	{
		uint64 lv = level;
		uint64 y = yLoc;
		y <<= (MAX_QTREE_LEVEL + 4);
		lv <<= (MAX_QTREE_LEVEL + MAX_QTREE_LEVEL + 8);
		return (uint64)(lv + y + xLoc);
	}


	template<class T>
	inline bool cQuadTree2<T>::Insert(qnode *node)
	{
		RETV(node->level >= MAX_QTREE_LEVEL, false);

		const uint64 key = MakeKey(node->level, node->xLoc, node->yLoc);

		auto it = m_nodeTable[node->level].find(key);
		if (m_nodeTable[node->level].end() != it)
			return false; // Error!! Already Exist

		m_nodeTable[node->level][key] = node;

		if (node->level == 0)
		{
			if (m_root)
				Remove(m_root);
			m_root = node;
		}

		return true;
	}


	template<class T>
	inline bool cQuadTree2<T>::Remove(qnode *node
		, const bool isRmTree //= true
	)
	{
		RETV(node->level >= MAX_QTREE_LEVEL, false);

		const uint64 key = MakeKey(node->level, node->xLoc, node->yLoc);

		auto it = m_nodeTable[node->level].find(key);
		if (m_nodeTable[node->level].end() == it)
			return false; // Error!! Not Exist

		for (int i = 0; i < 4; ++i)
			if (node->children[i])
				Remove(node->children[i], isRmTree);

		m_nodeTable[node->level].erase(key);

		if (isRmTree)
			delete node;

		return true;
	}


	// add children and allocate memory
	template<class T>
	inline bool cQuadTree2<T>::InsertChildren(qnode *parent)
	{
		RETV((parent->level + 1) >= MAX_QTREE_LEVEL, false);

		int locs[] = { 0,0, 1,0, 0,1, 1,1 }; //x,y loc
		for (int i = 0; i < 4; ++i)
		{
			qnode *p = new qnode;
			p->xLoc = (parent->xLoc << 1) + locs[i * 2];
			p->yLoc = (parent->yLoc << 1) + locs[i * 2 + 1];
			p->level = parent->level + 1;
			p->parent = parent;
			assert(!parent->children[i]);
			parent->children[i] = p;
			Insert(p);
		}
		return true;
	}


	// add children
	template<class T>
	inline bool cQuadTree2<T>::InsertChildren(
		qnode *parent, qnode *child[4])
	{
		RETV((parent->level + 1) >= MAX_QTREE_LEVEL, false);

		int locs[] = { 0,0, 1,0, 0,1, 1,1 }; //x,y loc
		for (int i = 0; i < 4; ++i)
		{
			qnode *p = child[i];
			p->xLoc = (parent->xLoc << 1) + locs[i * 2];
			p->yLoc = (parent->yLoc << 1) + locs[i * 2 + 1];
			p->level = parent->level + 1;
			p->parent = parent;
			assert(!parent->children[i]);
			parent->children[i] = p;
			Insert(p);
		}
		return true;
	}


	template<class T>
	inline bool cQuadTree2<T>::RemoveChildren(qnode *parent
		, const bool isRmTree //= true
	)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (parent->children[i])
			{
				Remove(parent->children[i], isRmTree);
				parent->children[i] = NULL;
			}
		}
		return true;
	}


	template<class T>
	inline sQNode<T>* cQuadTree2<T>::GetNode(const sRectf &rect)
	{
		const auto result = GetNodeLevel(rect);
		if (std::get<0>(result) < 0)
			return NULL;

		int nodeLevel = std::get<0>(result);
		int x = std::get<1>(result);
		int y = std::get<2>(result);

		qnode *ret = NULL;
		do
		{
			ret = GetNode(nodeLevel, x, y);
			if (nodeLevel <= 0)
				break;

			// goto parent level
			x >>= 1;
			y >>= 1;
			--nodeLevel;
		} while (ret == NULL);

		return ret;
	}


	// return node level, x, y correspond rect
	template<class T>
	inline std::tuple<int, int, int> cQuadTree2<T>::GetNodeLevel(const sRectf &rect)
	{
		if ((rect.right < 0) || (rect.bottom < 0))
			return std::make_tuple(-1, 0, 0);

		int x1 = (int)(rect.left);
		int y1 = (int)(rect.top);

		int xResult = x1 ^ ((int)(rect.right));
		int yResult = y1 ^ ((int)(rect.bottom));

		int nodeLevel = MAX_QTREE_LEVEL;
		int shiftCount = 0;

		while (xResult + yResult != 0)
		{
			xResult >>= 1;
			yResult >>= 1;
			nodeLevel--;
			shiftCount++;
		}

		x1 >>= shiftCount;
		y1 >>= shiftCount;
		return std::make_tuple(nodeLevel, x1, y1);
	}


	// return nodeLevel, x, y correspond rect and level
	template<class T>
	inline std::pair<int, int> cQuadTree2<T>::GetNodeLocation(
		const sRectf &rect, const int level)
	{
		const auto result = GetNodeLevel(rect);
		int lv = std::get<0>(result);
		int xLoc = std::get<1>(result);
		int yLoc = std::get<2>(result);

		if (lv < level)
			return std::make_pair(-1, -1); // error occurred!!

		// find level, xloc, yloc
		while (lv != level)
		{
			xLoc >>= 1;
			yLoc >>= 1;
			--lv;
		}
		return std::make_pair(xLoc, yLoc);
	}


	template<class T>
	inline sQNode<T>* cQuadTree2<T>::GetNode(
		const int level, const int xLoc, const int yLoc)
	{
		if ((uint)level >= MAX_QTREE_LEVEL)
			return NULL;
		const uint64 key = MakeKey(level, xLoc, yLoc);
		auto it = m_nodeTable[level].find(key);
		if (m_nodeTable[level].end() == it)
			return false; // Error!! Not Exist
		return it->second;
	}


	template<class T>
	inline sQNode<T>* cQuadTree2<T>::GetNode(const uint64 key)
	{
		const int level = (key >> (MAX_QTREE_LEVEL + MAX_QTREE_LEVEL + 8)) & 0x0F;
		if ((uint)level >= MAX_QTREE_LEVEL)
			return NULL;
		auto it = m_nodeTable[level].find(key);
		if (m_nodeTable[level].end() == it)
			return false; // Error!! Not Exist
		return m_nodeTable[level][key];
	}


	template<class T>
	inline sQNode<T>* cQuadTree2<T>::GetNorthNeighbor(const qnode *node)
	{
		sRectf rect = GetNodeGlobalRect(node);
		Vector2 pos = Vector2((float)rect.left, (float)rect.bottom);
		pos.y += (1.f / 2.f);

		qnode *ret = GetNode(sRectf::Rect(pos.x, pos.y, 0, 0));
		return (ret == node) ? NULL : ret;
	}


	template<class T>
	inline sQNode<T>* cQuadTree2<T>::GetSouthNeighbor(const qnode *node)
	{
		sRectf rect = GetNodeGlobalRect(node);
		Vector2 pos = Vector2((float)rect.left, (float)rect.top);
		pos.y -= (1.f / 2.f);

		qnode *ret = GetNode(sRectf::Rect(pos.x, pos.y, 0, 0));
		return (ret == node) ? NULL : ret;
	}


	template<class T>
	inline sQNode<T>* cQuadTree2<T>::GetWestNeighbor(const qnode *node)
	{
		sRectf rect = GetNodeGlobalRect(node);
		Vector2 pos = Vector2((float)rect.left, (float)rect.top);
		pos.x -= (1.f / 2.f);
		pos.y += (1.f / 2.f);

		qnode *ret = GetNode(sRectf::Rect(pos.x, pos.y, 0, 0));
		return (ret == node) ? NULL : ret;
	}


	template<class T>
	inline sQNode<T>* cQuadTree2<T>::GetEastNeighbor(const qnode *node)
	{
		sRectf rect = GetNodeGlobalRect(node);
		Vector2 pos = Vector2((float)rect.right, (float)rect.top);
		pos.x += (1.f / 2.f);
		pos.y += (1.f / 2.f);

		qnode *ret = GetNode(sRectf::Rect(pos.x, pos.y, 0, 0));
		return (ret == node) ? NULL : ret;
	}


	// return Relation Coordinate
	template<class T>
	inline sRectf cQuadTree2<T>::GetNodeRect(const int level, const int xLoc, const int yLoc)
	{
		sRectf rect;
		if (level >= 7)
		{
			const int clv = (level - 7);
			const int cxloc = 1088 << clv;
			const int cyloc = 442 << clv;
			const int xx = xLoc - cxloc;
			const int yy = yLoc - cyloc;
			const float size = (float)(1 << (16 - level));
			rect = sRectf::Rect(xx * size, yy*size, size, size);
		}
		else
		{
			const int clv = (7 - level);
			const int cxloc = xLoc << clv;
			const int cyloc = yLoc << clv;
			const int xx = cxloc - 1088;
			const int yy = cyloc - 442;
			const float size1 = (float)(1 << (16 - 7));
			const float size2 = (float)(1 << (16 - level));
			rect = sRectf::Rect(xx * size1, yy*size1, size2, size2);
		}

		return rect;
	}


	// return Relation Coordinate
	template<class T>
	inline sRectf cQuadTree2<T>::GetNodeRect(const qnode *node)
	{
		sRectf rect;
		if (node->level >= 7)
		{
			const int clv = (node->level - 7);
			const int cxloc = 1088 << clv;
			const int cyloc = 442 << clv;
			const int xx = node->xLoc - cxloc;
			const int yy = node->yLoc - cyloc;
			const float size = (float)(1 << (16 - node->level));
			rect = sRectf::Rect(xx * size, yy*size, size, size);
		}
		else
		{
			const int clv = (7 - node->level);
			const int cxloc = node->xLoc << clv;
			const int cyloc = node->yLoc << clv;
			const int xx = cxloc - 1088;
			const int yy = cyloc - 442;
			const float size1 = (float)(1 << (16 - 7));
			const float size2 = (float)(1 << (16 - node->level));
			rect = sRectf::Rect(xx * size1, yy*size1, size2, size2);
		}

		return rect;
	}


	// return Global Coordinate
	template<class T>
	inline sRectf cQuadTree2<T>::GetNodeGlobalRect(const qnode *node)
	{
		const float size = (float)(1 << (16 - node->level));
		const sRectf rect = sRectf::Rect(node->xLoc * size, node->yLoc*size, size, size);
		return rect;
	}


	// covert relation coordinate to global coordinate
	template<class T>
	inline Vector3 cQuadTree2<T>::GetGlobalPos(const Vector3 &relPos)
	{
		const float size = (float)(1 << (16 - 7));
		const int xLoc = 1088;
		const int yLoc = 442;
		const float offsetX = size * xLoc;
		const float offsetY = size * yLoc;
		return Vector3(relPos.x + offsetX, relPos.y, relPos.z + offsetY);
	}


	// covert relation coordinate to global coordinate
	template<class T>
	inline Vector3 cQuadTree2<T>::GetRelationPos(const Vector3 &globalPos)
	{
		const float size = (float)(1 << (16 - 7));
		const int xLoc = 1088;
		const int yLoc = 442;
		const float offsetX = size * xLoc;
		const float offsetY = size * yLoc;
		return Vector3(globalPos.x - offsetX, globalPos.y, globalPos.z - offsetY);
	}


	template<class T>
	inline void cQuadTree2<T>::Clear(
		const bool isRmTree //= true
	)
	{
		for (int i = 0; i < MAX_QTREE_LEVEL; ++i)
		{
			for (auto kv : m_nodeTable[i])
				if (isRmTree)
					delete kv.second;
			m_nodeTable[i].clear();
		}
		m_root = nullptr;
	}

}
