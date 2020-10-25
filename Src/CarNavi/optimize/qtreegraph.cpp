
#include "stdafx.h"
#include "qtreegraph.h"

using namespace optimize;


cQTreeGraph::cQTreeGraph()
	: m_isDivide(false)
{
}

cQTreeGraph::~cQTreeGraph()
{
	Clear();
}


// add point
// pos: relation pos
qgid cQTreeGraph::AddPoint(const Vector3 &pos)
{
	const Vector3 gpos = cQuadTree<>::GetGlobalPos(pos);
	const sRectf rect = sRectf::Rect(gpos.x, gpos.z, 0, 0);
	int level = TREE_LEVEL;
	const auto result = cQuadTree<>::GetNodeLocation(rect, level);
	int xLoc = std::get<0>(result);
	int yLoc = std::get<1>(result);
	if (xLoc < 0)
		return 0; // error occurred!!

	cQuadTree<sNode> *qtree = FindAndCreateTree(level, xLoc, yLoc);
	if (!qtree)
		return 0; // error occurred!!

	m_isDivide = false;
	return AddPointInBestNode(qtree, rect, pos);
}


// find quadtree from lv, xloc, yloc
// if not exist, create quadtree
cQuadTree<sNode>* cQTreeGraph::FindAndCreateTree(const int lv
	, const int xLoc, const int yLoc)
{
	const uint64 key = cQuadTree<>::MakeKey(lv, xLoc, yLoc);
	auto it = m_qtrees.find(key);
	if (m_qtrees.end() != it)
		return it->second;

	cQuadTree<sNode> *newQtree = new cQuadTree<sNode>();
	m_qtrees[key] = newQtree;

	sQuadTreeNode<sNode> *root = new sQuadTreeNode<sNode>();
	root->level = lv;
	root->xLoc = xLoc;
	root->yLoc = yLoc;
	newQtree->m_rootRect = cQuadTree<sNode>::GetNodeRect(lv, xLoc, yLoc);
	newQtree->Insert(root);
	newQtree->m_roots.push_back(root); // tricky code
	return newQtree;
}


// find root tree, level:TREE_LEVEL
cQuadTree<sNode>* cQTreeGraph::FindRootTree(const int level
	, const int xLoc, const int yLoc)
{
	if (level != TREE_LEVEL)
	{
		int lv = level;
		int x = xLoc;
		int y = yLoc;
		while (lv != TREE_LEVEL)
		{
			--lv;
			x >>= 1;
			y >>= 1;
		}
		const uint64 key = cQuadTree<>::MakeKey(lv, x, y);
		auto it = m_qtrees.find(key);
		if (m_qtrees.end() != it)
			return it->second;
	}
	else
	{
		const uint64 key = cQuadTree<>::MakeKey(level, xLoc, yLoc);
		auto it = m_qtrees.find(key);
		if (m_qtrees.end() != it)
			return it->second;
	}
	return nullptr;
}


// find best fit node
sQuadTreeNode<sNode>* cQTreeGraph::FindBestNode(const int level
	, const int xLoc, const int yLoc)
{
	cQuadTree<sNode> *root = nullptr;

	// check root node
	if (level >= TREE_LEVEL)
	{
		int lv = level;
		int x = xLoc;
		int y = yLoc;
		while (lv != TREE_LEVEL)
		{
			--lv;
			x >>= 1;
			y >>= 1;
		}
		const uint64 key = cQuadTree<>::MakeKey(lv, x, y);
		auto it = m_qtrees.find(key);
		if (m_qtrees.end() != it)
		{
			root = it->second;
			cQuadTree<sNode> *qtree = it->second;
			sQuadTreeNode<sNode> *node = qtree->m_roots[0];

			// leaf node? return this node
			if (node->children[0] == nullptr)
				return node;
		}
	}

	if (!root)
		return nullptr; // error occurred!!

	int lv = level;
	int x = xLoc;
	int y = yLoc;
	while (lv != TREE_LEVEL)
	{
		const uint64 key = cQuadTree<>::MakeKey(lv, x, y);
		sQuadTreeNode<sNode> *node = root->GetNode(key);
		if (node)
			return node;
		--lv;
		x >>= 1;
		y >>= 1;
	}

	return nullptr; // error occurred!!
}


// find node
sQuadTreeNode<sNode>* cQTreeGraph::FindNode(const qgid id)
{
	int index, level, xLoc, yLoc;
	PARSE_QGID(id, index, level, xLoc, yLoc);
	if (level < TREE_LEVEL)
		return nullptr; // error occurred

	cQuadTree<sNode> *qtree = FindRootTree(level, xLoc, yLoc);
	if (!qtree)
		return nullptr;
	return qtree->GetNode(level, xLoc, yLoc);
}


// get vertex position
Vector3 cQTreeGraph::GetVertexPos(const qgid id)
{
	sQuadTreeNode<sNode> *node = FindNode(id);
	if (!node)
	{
		int index, level, xloc, yloc;
		PARSE_QGID(id, index, level, xloc, yloc);
		return Vector3(); // error occurred!!
	}
	
	//int index;
	//PARSE_QGID_INDEX(id, index);
	int index, level, xloc, yloc;
	PARSE_QGID(id, index, level, xloc, yloc);
	return node->data.table[index].pos;
}


// add point in best quadtree node
// pos: relation pos
qgid cQTreeGraph::AddPointInBestNode(cQuadTree<sNode> *qtree
	, const sRectf &rect, const Vector3 &pos)
{
	m_mappingIds.clear();

	sQuadTreeNode<sNode> *rootNode = qtree->m_roots[0];
	int level = rootNode->level;
	int xLoc = rootNode->xLoc;
	int yLoc = rootNode->yLoc;
	Vector3 retVal = pos;
	qgid ret = 0;

	// find best tree node
	sQuadTreeNode<sNode> *node = qtree->m_roots[0];
	while (1)
	{
		const bool hasChild = node->children[0] != nullptr;
		if ((hasChild || (node->data.table.size() > MAX_TABLESIZE))
			&& (level < (cQuadTree<>::MAX_LEVEL - 1)))
		{
			++level;
			const auto res = cQuadTree<>::GetNodeLocation(rect, level);
			int x = std::get<0>(res);
			int y = std::get<1>(res);
			sQuadTreeNode<sNode> *child = qtree->GetNode(level, x, y);
			if (child)
			{
				node = child;
				if (child->data.table.size() > MAX_TABLESIZE)
					continue; // goto children
				if (child->children[0] != nullptr)
					continue; // goto children
			}
			else
			{
				// make child node and table
				DivideNodeToChild(qtree, node);

				node = qtree->GetNode(level, x, y);
				continue; // goto children
			}
		}
		if (!node)
			break; // error occurred!!

		// find duplicate pos
		bool isFind = false;
		for (uint i=0; i < node->data.table.size(); ++i)
		{
			sAccPos &apos = node->data.table[i];
			const Vector3 p = apos.pos;
			const float dist = pos.Distance(p);

			// same position?
			if (dist < 0.1f)
			{
				// average position, update
				apos.pos = ((p * (float)apos.cnt) / (float)(apos.cnt + 1))
					+ (pos / (float)(apos.cnt + 1));
				apos.cnt += 1;
				retVal = apos.pos;
				isFind = true;
				MAKE_QGID(ret, (int)i, node->level, node->xLoc, node->yLoc);
				break;
			}
		}

		// no near position, add new pos
		if (!isFind)
		{
			node->data.table.push_back({ 1, pos });
			MAKE_QGID(ret, (int)node->data.table.size() - 1
				, node->level, node->xLoc, node->yLoc);
		}

		break; // complete
	}

	return ret;
}


// dived quadtree node to child node
// table, graph divided to child node
// and remove current node table, graph info
bool cQTreeGraph::DivideNodeToChild(cQuadTree<sNode> *qtree
	, sQuadTreeNode<sNode> *node)
{
	m_isDivide = true;
	qtree->InsertChildren(node);

	map<qgid, qgid> ids; // key: old id, value: new id

	const int nextLv = node->level + 1;

	// table divide
	for (uint i=0; i < node->data.table.size(); ++i)
	{
		auto &apos = node->data.table[i];
		const Vector3 gp = cQuadTree<>::GetGlobalPos(apos.pos);
		const sRectf r = sRectf::Rect(gp.x, gp.z, 0, 0);
		const auto result = cQuadTree<>::GetNodeLocation(r, nextLv);
		const int xLoc = std::get<0>(result);
		const int yLoc = std::get<1>(result);

		sQuadTreeNode<sNode> *pn = qtree->GetNode(nextLv, xLoc, yLoc);
		if (!pn)
			return false; // error occurred!!

		pn->data.table.push_back(apos);

		const int newIdx = pn->data.table.size() - 1;

		// mapping current id, new id
		qgid id0, id1;
		MAKE_QGID(id0, i, node->level, node->xLoc, node->yLoc);
		MAKE_QGID(id1, newIdx, nextLv, xLoc, yLoc);

		ids[id0] = id1;
	}

	// graph divide
	for (uint i = 0; i < node->data.vertices.size(); ++i)
	{
		auto &vtx = node->data.vertices[i];

		auto it = ids.find(vtx.id);
		if (ids.end() == it)
			continue; // error occurred!!

		const qgid newId = it->second;
		int index, level, xLoc, yLoc;
		PARSE_QGID(newId, index, level, xLoc, yLoc);

		sQuadTreeNode<sNode> *pn = qtree->GetNode(nextLv, xLoc, yLoc);
		if (!pn)
			return false; // error occurred!!

		sVertex newVtx = vtx; // memory move
		newVtx.id = newId;
		
		for (uint k = 0; k < newVtx.trCnt; ++k)
		{
			sTransition &tr = newVtx.trs[k];			
			auto it0 = ids.find(tr.to);
			if (ids.end() == it0)
			{
				// another node vertex
				sVertex *toVtx = FindVertex(tr.to);
				if (!toVtx)
					continue; // error occurred!!

				// update connected vertex transition
				for (uint m = 0; m < toVtx->trCnt; ++m)
				{
					if (toVtx->trs[m].to == vtx.id)
					{
						toVtx->trs[m].to = newId;
						break;
					}
				}
			}
			else
			{
				// same quad tree node, update id
				tr.to = it0->second;
			}
		}

		vtx.trCnt = 0;
		vtx.trs = nullptr; // memory move, so null
		pn->data.vertices.push_back(newVtx);
	}

	// clear current node data
	node->data.table.clear();
	node->data.table.shrink_to_fit(); // clear memory
	node->data.vertices.clear();
	node->data.vertices.shrink_to_fit(); // clear memory

	if (m_mappingIds.empty())
	{
		m_mappingIds = ids; // store mapping ids
	}
	else
	{
		// multiple link
		for (auto &kv : m_mappingIds)
			kv.second = ids[kv.second];
	}
	return true;
}


// make id
// index + level + xLoc + yLoc
qgid cQTreeGraph::MakeQgid(const int index
	, const int level, const int xLoc, const int yLoc)
{
	const int maxLv = cQuadTree<>::MAX_LEVEL;
	const uint64 idx = (uint64)index << (maxLv + maxLv + 2 + 4);
	const uint64 lv= (uint64)level << (maxLv + maxLv + 2);
	const uint64 y = (uint64)yLoc << (maxLv + 1);
	return idx + lv + y + xLoc;
}


// parse id
// id -> index + level + xloc + yloc
// return : index, level, xloc, yloc
std::tuple<int, int, int, int> cQTreeGraph::ParseQgid(const qgid id)
{
	const int maxLv = cQuadTree<>::MAX_LEVEL;
	const uint64 idx = id >> (maxLv + maxLv + 2 + 4);
	const uint64 lv = (id >> (maxLv + maxLv + 2)) & 0xF;
	const uint64 yloc = (id >> (maxLv + 1)) & 0xFFFF;
	const uint64 xloc = id & 0xFFFF;
	return std::make_tuple((int)idx, (int)lv, (int)xloc, (int)yloc);
}


// return quadtree id (lv + xloc + yloc) from qaudtree graph id
uint64 cQTreeGraph::GetQTreeIdFromQgid(const qgid id)
{
	// lv:4bit, xloc:17bit, yloc:17bit => 0x3FFFFFFFFF
	uint64 qid = id & (uint64)(0x3FFFFFFFFF);
	return qid;
}


// add transition
// id0 : position id0 
// id1 : position id1
bool cQTreeGraph::AddTransition(const qgid id0, const qgid id1)
{
	if (id0 == id1)
		return false; // same point

	sQuadTreeNode<sNode> *node0 = FindNode(id0);
	sQuadTreeNode<sNode> *node1 = FindNode(id1);
	if (!node0 || !node1)
		return false; // error occurred!!

	// check transition length
	{
		int idx00, lv00, xloc00, yloc00;
		int idx01, lv01, xloc01, yloc01;
		PARSE_QGID(id0, idx00, lv00, xloc00, yloc00);
		PARSE_QGID(id1, idx01, lv01, xloc01, yloc01);

		int idx0, idx1;
		PARSE_QGID_INDEX(id0, idx0);
		PARSE_QGID_INDEX(id1, idx1);
		const float len = node0->data.table[idx0].pos.Distance(
			node1->data.table[idx1].pos
		);
		if (len > 50.0f)
			return false; // error, too long
	}

	sVertex *vtx0 = FindVertex(node0, id0);
	sVertex *vtx1 = FindVertex(node1, id1);
	if (!vtx0)
	{
		sVertex vtx;
		vtx.id = id0;
		node0->data.vertices.push_back(vtx);
	}

	if (!vtx1)
	{
		sVertex vtx;
		vtx.id = id1;
		node1->data.vertices.push_back(vtx);
	}

	vtx0 = FindVertex(node0, id0);
	vtx1 = FindVertex(node1, id1);
	if (!vtx0 || !vtx0)
		return false; // error occurred!!

	sTransition *ptr = FindTransition(vtx0, id1);
	if (ptr)
		return false; // already exist transition

	// add bidirection
	sTransition tr0;
	tr0.to = id1;
	if (!vtx0->trs)
	{
		vtx0->trCnt = 0;
		vtx0->trCapa = sVertex::DEFAULT_TRANSITION;
		vtx0->trs = new sTransition[sVertex::DEFAULT_TRANSITION];
		ZeroMemory(vtx0->trs, sizeof(sTransition) * sVertex::DEFAULT_TRANSITION);		
	}
	AddVertexTransition(vtx0, tr0);

	sTransition tr1;
	tr1.to = id0;
	if (!vtx1->trs)
	{
		vtx1->trCnt = 0;
		vtx1->trCapa = sVertex::DEFAULT_TRANSITION;
		vtx1->trs = new sTransition[sVertex::DEFAULT_TRANSITION];
		ZeroMemory(vtx1->trs, sizeof(sTransition) * sVertex::DEFAULT_TRANSITION);
	}
	AddVertexTransition(vtx1, tr1);

	return true;
}


// find near edge
// distance : max distance
// return : near edge
//			no near edge : {0,0}
sEdge cQTreeGraph::FindNearEdge(const Vector3 &pos, const float distance)
{
	const Vector3 gpos = cQuadTree<>::GetGlobalPos(pos);
	const sRectf rect = sRectf::Rect(gpos.x, gpos.z, 0, 0);
	int level = TREE_LEVEL;
	const auto result = cQuadTree<>::GetNodeLocation(rect, level);
	int xLoc = std::get<0>(result);
	int yLoc = std::get<1>(result);
	if (xLoc < 0)
		return {0,0}; // error occurred!!

	sQuadTreeNode<sNode> *node = FindBestNode(level, xLoc, yLoc);
	if (!node)
		return {0,0}; // error occurred!!

	float minLen = FLT_MAX;
	sEdge edge;
	for (auto &vtx : node->data.vertices)
	{
		int idx0, lv0, x0, y0;
		PARSE_QGID(vtx.id, idx0, lv0, x0, y0);
		const Vector3 &p0 = node->data.table[idx0].pos;

		for (uint i = 0; i < vtx.trCnt; ++i)
		{
			Vector3 p1;
			const sTransition &tr = vtx.trs[i];
			if (COMPARE_QID(vtx.id, tr.to))
			{
				int idx1;
				PARSE_QGID_INDEX(tr.to, idx1);
				p1 = node->data.table[idx1].pos;
			}
			else
			{
				p1 = GetVertexPos(tr.to);
			}

			const Line line(p0, p1);
			const float len = line.GetDistance(pos);
			if ((len < distance) && (len < minLen))
			{
				minLen = len;
				edge = sEdge(vtx.id, tr.to);
			}
		}
	}

	return edge;
}


// smooth edge with pos
//	- insertion
//	- average smooth
bool cQTreeGraph::SmoothEdge(const Vector3 &pos, const sEdge &edge)
{
	sQuadTreeNode<sNode> *node0 = FindNode(edge.from);
	sQuadTreeNode<sNode> *node1 = nullptr;
	if (COMPARE_QID(edge.from, edge.to))
		node1 = node0;
	else
		node1 = FindNode(edge.to);
	if (!node0 || !node1)
		return false; // error occurred!!

	int idx0, idx1;
	PARSE_QGID_INDEX(edge.from, idx0);
	PARSE_QGID_INDEX(edge.to, idx1);

	sAccPos &apos0 = node0->data.table[idx0];
	sAccPos &apos1 = node1->data.table[idx1];
	apos0.cnt++;
	apos1.cnt++;

	const Vector3 p0 = (pos / (float)apos0.cnt) + 
		((apos0.pos * (float)(apos0.cnt - 1)) / (float)apos0.cnt);
	const Vector3 p1 = (pos / (float)apos1.cnt) +
		((apos1.pos * (float)(apos1.cnt - 1)) / (float)apos1.cnt);
	apos0.pos = p0;
	apos1.pos = p1;
	return true;
}


// find vertex from qgid
sVertex* cQTreeGraph::FindVertex(const qgid id)
{
	int index, level, xLoc, yLoc;
	PARSE_QGID(id, index, level, xLoc, yLoc);

	cQuadTree<sNode> *qtree = FindRootTree(level, xLoc, yLoc);
	if (!qtree)
		return nullptr;

	sQuadTreeNode<sNode> *node = qtree->GetNode(level, xLoc, yLoc);
	if (!node)
		return nullptr;

	for (auto &vtx : node->data.vertices)
	{
		if (vtx.id == id)
			return &vtx;
	}
	return nullptr;
}


// find vertex
sVertex* cQTreeGraph::FindVertex(sQuadTreeNode<sNode>*node, const qgid id)
{
	for (auto &vtx : node->data.vertices)
		if (vtx.id == id)
			return &vtx;
	return nullptr;
}


// find transition
// toId: to vertex id
sTransition* cQTreeGraph::FindTransition(sVertex *vtx, const qgid toId)
{
	if (!vtx->trs)
		return nullptr;
	for (uint i = 0; i < vtx->trCnt; ++i)
		if (vtx->trs[i].to == toId)
			return &vtx->trs[i];
	return nullptr;
}


// add vertex transition
bool cQTreeGraph::AddVertexTransition(sVertex *vtx, const sTransition &tr)
{
	if (vtx->trCapa == vtx->trCnt)
	{
		// increase memory
		sTransition *p = new sTransition[vtx->trCapa * 2];
		memcpy(p, vtx->trs, sizeof(sTransition) * vtx->trCnt);
		delete[] vtx->trs;
		vtx->trs = p;
		vtx->trCapa *= 2;
	}

	vtx->trs[vtx->trCnt] = tr;
	vtx->trCnt++;
	return true;
}


// generate graph vertex, transition line
bool cQTreeGraph::CreateGraphLines(graphic::cRenderer &renderer
	, sQuadTreeNode<sNode> *node)
{
	using namespace graphic;

	if (node->data.lineList)
		return true; // already exist

	int maxLine = 0;
	for (auto &vtx : node->data.vertices)
		maxLine += vtx.trCnt;
	if (maxLine == 0)
		return true;

	node->data.lineList = new cDbgLineList();
	node->data.lineList->Create(renderer, maxLine, cColor::WHITE);

	for (auto &vtx : node->data.vertices)
	{
		int idx0, lv0, x0, y0;
		PARSE_QGID(vtx.id, idx0, lv0, x0, y0);
		const Vector3 &p0 = node->data.table[idx0].pos;

		for (uint i = 0; i < vtx.trCnt; ++i)
		{
			Vector3 p1;
			const sTransition &tr = vtx.trs[i];
			if (COMPARE_QID(vtx.id, tr.to))
			{
				int idx1;
				PARSE_QGID_INDEX(tr.to, idx1);
				p1 = node->data.table[idx1].pos;
			}
			else
			{
				p1 = GetVertexPos(tr.to);
			}
			node->data.lineList->AddLine(renderer, p0, p1, false);
		}
	}
	node->data.lineList->UpdateBuffer(renderer);
	return true;
}


void cQTreeGraph::Clear()
{
	for (auto &kv : m_qtrees)
	{
		cQuadTree<sNode> *qtree = kv.second;
		for (int i = 0; i < cQuadTree<>::MAX_LEVEL; ++i)
		{
			std::map<uint64, sQuadTreeNode<sNode>*> &m = qtree->m_nodeTable[i];
			for (auto &kv2 : m)
			{
				sQuadTreeNode<sNode> *node = kv2.second;
				for (auto &vtx : node->data.vertices)
				{
					SAFE_DELETEA(vtx.trs);
					vtx.trCnt = 0;
					vtx.trCapa = 0;
				}
				SAFE_DELETE(node->data.lineList);
			}
		}
		delete kv.second;
	}
	m_qtrees.clear();
}

