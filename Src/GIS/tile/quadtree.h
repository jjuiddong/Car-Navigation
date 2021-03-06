//
// 2017-12-14, jjuiddong
// Quad-Tree for Geometry (Sparse QuadTree)
//
//- Game Programming Gems2, 4.5
//	- Direct Access QuadTree Lookup
//
#pragma once


struct eDirection 
{ 
	enum Enum { NORTH, EAST, SOUTH, WEST }; 
};


template<class T>
struct sQuadTreeNode : public common::cMemoryPool<sQuadTreeNode<T>>
{
	int xLoc;
	int yLoc;
	int level;
	sQuadTreeNode *parent; // reference
	sQuadTreeNode *children[4]; // reference
	T data;

	sQuadTreeNode() {
		xLoc = 0;
		yLoc = 0;
		level = 0;
		parent = nullptr;
		children[0] = children[1] = children[2] = children[3] = nullptr;
	}
};


template<class T = int> // int : dummy
class cQuadTree
{
public:
	cQuadTree();
	~cQuadTree();

	bool Insert(sQuadTreeNode<T> *node);
	bool Remove(sQuadTreeNode<T> *node, const bool isRmTree = true);
	bool InsertChildren(sQuadTreeNode<T> *node);
	bool InsertChildren(sQuadTreeNode<T> *node, sQuadTreeNode<T> *child[4]);
	bool RemoveChildren(sQuadTreeNode<T> *node, const bool isRmTree = true);
	sQuadTreeNode<T>* GetNode(const sRectf &rect);
	sQuadTreeNode<T>* GetNode(const int level, const int xLoc, const int yLoc);
	sQuadTreeNode<T>* GetNode(const uint64 key);
	sQuadTreeNode<T>* GetNorthNeighbor(const sQuadTreeNode<T> *node);
	sQuadTreeNode<T>* GetSouthNeighbor(const sQuadTreeNode<T> *node);
	sQuadTreeNode<T>* GetWestNeighbor(const sQuadTreeNode<T> *node);
	sQuadTreeNode<T>* GetEastNeighbor(const sQuadTreeNode<T> *node);

	static std::tuple<int,int,int> GetNodeLevel(const sRectf &rect);
	static std::pair<int, int> GetNodeLocation(const sRectf &rect, const int level);
	static sRectf GetNodeRect(const int level, const int xLoc, const int yLoc);
	static sRectf GetNodeRect(const sQuadTreeNode<T> *node);
	static sRectf GetNodeGlobalRect(const sQuadTreeNode<T> *node);
	static Vector3 GetGlobalPos(const Vector3 &relPos);
	static Vector3 GetRelationPos(const Vector3 &globalPos);
	static uint64 MakeKey(const int level, const int xLoc, const int yLoc);
	void Clear(const bool isRmTree = true);


public:
	enum {MAX_LEVEL = 16};

	vector<sQuadTreeNode<T>*> m_roots; // root nodes (multiple root node)
	sRectf m_rootRect;
	static const float m_quadScale;
	std::map<uint64, sQuadTreeNode<T>*> m_nodeTable[MAX_LEVEL]; // level quad tree map
														  // key = quad node x,y index (linear)
};
template<class T> const float cQuadTree<T>::m_quadScale = 1.f;


// m_nodeTable key
// Level            Key
//   0               0
//   1       0       |        1       |       2      |        3
//   2    0,1,2,3    |    4,5,6,7     |  8,9,10,11   |   12,13,14,15
//   3    ....

template<class T>
inline cQuadTree<T>::cQuadTree()
{
	m_rootRect = sRectf::Rect(0, 0, 1 << MAX_LEVEL, 1 << MAX_LEVEL);
}

template<class T>
inline cQuadTree<T>::~cQuadTree()
{
	Clear();
}


template<class T>
inline uint64 cQuadTree<T>::MakeKey(const int level, const int xLoc, const int yLoc)
{
	uint64 lv = level;
	uint64 y = yLoc;
	y <<= (MAX_LEVEL + 4);
	lv <<= (MAX_LEVEL + MAX_LEVEL + 8);
	return (uint64)(lv + y + xLoc);
}


template<class T>
inline bool cQuadTree<T>::Insert(sQuadTreeNode<T> *node)
{
	RETV(node->level >= MAX_LEVEL, false);

	const uint64 key = MakeKey(node->level, node->xLoc, node->yLoc);

	auto it = m_nodeTable[node->level].find(key);
	if (m_nodeTable[node->level].end() != it)
		return false; // Error!! Already Exist

	m_nodeTable[node->level][key] = node;

	if (node->level == 0)
		m_roots.push_back(node);

	return true;
}


// todo: clear root, parent reference
template<class T>
inline bool cQuadTree<T>::Remove(sQuadTreeNode<T> *node
	, const bool isRmTree //= true
)
{
	RETV(node->level >= MAX_LEVEL, false);

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
inline bool cQuadTree<T>::InsertChildren(sQuadTreeNode<T> *parent)
{
	RETV((parent->level + 1) >= MAX_LEVEL, false);

	int locs[] = { 0,0, 1,0, 0,1, 1,1 }; //x,y loc
	for (int i = 0; i < 4; ++i)
	{
		sQuadTreeNode<T> *p = new sQuadTreeNode<T>;
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
inline bool cQuadTree<T>::InsertChildren(
	sQuadTreeNode<T> *parent, sQuadTreeNode<T> *child[4])
{
	RETV((parent->level + 1) >= MAX_LEVEL, false);

	int locs[] = { 0,0, 1,0, 0,1, 1,1 }; //x,y loc
	for (int i = 0; i < 4; ++i)
	{
		sQuadTreeNode<T> *p = child[i];
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
inline bool cQuadTree<T>::RemoveChildren(sQuadTreeNode<T> *parent
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
inline sQuadTreeNode<T>* cQuadTree<T>::GetNode(const sRectf &rect)
{
	const auto result = GetNodeLevel(rect);
	if (std::get<0>(result) < 0)
		return NULL;

	int nodeLevel = std::get<0>(result);
	int x = std::get<1>(result);
	int y = std::get<2>(result);

	sQuadTreeNode<T> *ret = NULL;
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
inline std::tuple<int, int, int> cQuadTree<T>::GetNodeLevel(const sRectf &rect)
{
	//if ((rect.left > m_rootRect.right) || (rect.top > m_rootRect.bottom)
	//	|| (rect.right < 0) || (rect.bottom < 0))
	//	return std::make_tuple(-1,0,0);
	if ((rect.right < 0) || (rect.bottom < 0))
		return std::make_tuple(-1,0,0);

	int x1 = (int)(rect.left * m_quadScale);
	int y1 = (int)(rect.top * m_quadScale);

	int xResult = x1 ^ ((int)(rect.right * m_quadScale));
	int yResult = y1 ^ ((int)(rect.bottom * m_quadScale));

	int nodeLevel = MAX_LEVEL;
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
inline std::pair<int, int> cQuadTree<T>::GetNodeLocation(
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
inline sQuadTreeNode<T>* cQuadTree<T>::GetNode(
	const int level, const int xLoc, const int yLoc)
{
	if ((uint)level >= MAX_LEVEL)
		return nullptr;
	const uint64 key = MakeKey(level, xLoc, yLoc);
	auto it = m_nodeTable[level].find(key);
	if (m_nodeTable[level].end() == it)
		return nullptr; // Error!! Not Exist
	return it->second;
}


template<class T>
inline sQuadTreeNode<T>* cQuadTree<T>::GetNode(const uint64 key)
{
	//const int level = (key & 0x3C00000000) >> (MAX_LEVEL + MAX_LEVEL + 2);
	const int level = (key >> (MAX_LEVEL + MAX_LEVEL + 8)) & 0x0F;
	if ((uint)level >= MAX_LEVEL)
		return nullptr;
	auto it = m_nodeTable[level].find(key);
	if (m_nodeTable[level].end() == it)
		return nullptr; // Error!! Not Exist
	return m_nodeTable[level][key];
}


template<class T>
inline sQuadTreeNode<T>* cQuadTree<T>::GetNorthNeighbor(const sQuadTreeNode<T> *node)
{
	sRectf rect = GetNodeGlobalRect(node);
	Vector2 pos = Vector2((float)rect.left, (float)rect.bottom);
	pos.y += (m_quadScale / 2.f);

	sQuadTreeNode<T> *ret = GetNode(sRectf::Rect(pos.x, pos.y, 0, 0));
	return (ret == node) ? nullptr : ret;
}


template<class T>
inline sQuadTreeNode<T>* cQuadTree<T>::GetSouthNeighbor(const sQuadTreeNode<T> *node)
{
	sRectf rect = GetNodeGlobalRect(node);
	Vector2 pos = Vector2((float)rect.left, (float)rect.top);
	pos.y -= (m_quadScale / 2.f);

	sQuadTreeNode<T> *ret = GetNode(sRectf::Rect(pos.x, pos.y, 0, 0));
	return (ret == node) ? NULL : ret;
}


template<class T>
inline sQuadTreeNode<T>* cQuadTree<T>::GetWestNeighbor(const sQuadTreeNode<T> *node)
{
	sRectf rect = GetNodeGlobalRect(node);
	Vector2 pos = Vector2((float)rect.left, (float)rect.top);
	pos.x -= (m_quadScale / 2.f);
	pos.y += (m_quadScale / 2.f);

	sQuadTreeNode<T> *ret = GetNode(sRectf::Rect(pos.x, pos.y, 0, 0));
	return (ret == node) ? NULL : ret;
}


template<class T>
inline sQuadTreeNode<T>* cQuadTree<T>::GetEastNeighbor(const sQuadTreeNode<T> *node)
{
	sRectf rect = GetNodeGlobalRect(node);
	Vector2 pos = Vector2((float)rect.right, (float)rect.top);
	pos.x += (m_quadScale / 2.f);
	pos.y += (m_quadScale / 2.f);

	sQuadTreeNode<T> *ret = GetNode(sRectf::Rect(pos.x, pos.y, 0, 0));
	return (ret == node) ? NULL : ret;
}


// return Relation Coordinate
template<class T>
inline sRectf cQuadTree<T>::GetNodeRect(const int level, const int xLoc, const int yLoc)
{
	sRectf rect;
	if (level >= 7)
	{
		const int clv = (level - 7);
		const int cxloc = 1088 << clv;
		const int cyloc = 442 << clv;
		const int xx = xLoc - cxloc;
		const int yy = yLoc - cyloc;
		const float size = (1 << (16 - level)) * m_quadScale;
		rect = sRectf::Rect(xx * size, yy*size, size, size);
	}
	else
	{
		const int clv = (7 - level);
		const int cxloc = xLoc << clv;
		const int cyloc = yLoc << clv;
		const int xx = cxloc - 1088;
		const int yy = cyloc - 442;
		const float size1 = (1 << (16 - 7)) * m_quadScale;
		const float size2 = (1 << (16 - level)) * m_quadScale;
		rect = sRectf::Rect(xx * size1, yy*size1, size2, size2);
	}

	return rect;
}


// return Relation Coordinate
template<class T>
inline sRectf cQuadTree<T>::GetNodeRect(const sQuadTreeNode<T> *node)
{
	sRectf rect;
	if (node->level >= 7)
	{
		const int clv = (node->level - 7);
		const int cxloc = 1088 << clv;
		const int cyloc = 442 << clv;
		const int xx = node->xLoc - cxloc;
		const int yy = node->yLoc - cyloc;
		const float size = (1 << (16 - node->level)) * m_quadScale;
		rect = sRectf::Rect(xx * size, yy*size, size, size);
	}
	else
	{
		const int clv = (7 - node->level);
		const int cxloc = node->xLoc << clv;
		const int cyloc = node->yLoc << clv;
		const int xx = cxloc - 1088;
		const int yy = cyloc - 442;
		const float size1 = (1 << (16 - 7)) * m_quadScale;
		const float size2 = (1 << (16 - node->level)) * m_quadScale;
		rect = sRectf::Rect(xx * size1, yy*size1, size2, size2);
	}

	return rect;
}


// return Global Coordinate
template<class T>
inline sRectf cQuadTree<T>::GetNodeGlobalRect(const sQuadTreeNode<T> *node)
{
	const float size = (1 << (16 - node->level)) * m_quadScale;
	const sRectf rect = sRectf::Rect(node->xLoc * size, node->yLoc*size, size, size);
	return rect;
}


// covert relation coordinate to global coordinate
template<class T>
inline Vector3 cQuadTree<T>::GetGlobalPos(const Vector3 &relPos)
{
	const float size = (1 << (16 - 7)) * m_quadScale;
	const int xLoc = 1088;
	const int yLoc = 442;
	const float offsetX = size * xLoc;
	const float offsetY = size * yLoc;
	return Vector3(relPos.x + offsetX, relPos.y, relPos.z + offsetY);
}


// covert relation coordinate to global coordinate
template<class T>
inline Vector3 cQuadTree<T>::GetRelationPos(const Vector3 &globalPos)
{
	const float size = (1 << (16 - 7)) * m_quadScale;
	const int xLoc = 1088;
	const int yLoc = 442;
	const float offsetX = size * xLoc;
	const float offsetY = size * yLoc;
	return Vector3(globalPos.x - offsetX, globalPos.y, globalPos.z - offsetY);
}


template<class T>
inline void cQuadTree<T>::Clear(
	const bool isRmTree //= true
)
{
	for (int i = 0; i < MAX_LEVEL; ++i)
	{
		for (auto kv : m_nodeTable[i])
			if (isRmTree)
				delete kv.second;
		m_nodeTable[i].clear();
	}
	m_roots.clear();
}
