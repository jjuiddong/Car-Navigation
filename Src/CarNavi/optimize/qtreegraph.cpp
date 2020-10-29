
#include "stdafx.h"
#include "qtreegraph.h"
#include <direct.h>

using namespace optimize;


cQTreeGraph::cQTreeGraph()
	: m_isDivide(false)
{
}

cQTreeGraph::~cQTreeGraph()
{
	Clear();
}


// read quadtree graph data
bool cQTreeGraph::ReadFile()
{
	// calc max level
	int maxLevel = -1;
	for (int i = cQTreeGraph::DEFAULT_TREE_LEVEL; i < cQuadTree<>::MAX_LEVEL; ++i)
	{
		StrPath path;
		path.Format("%s\\%d", s_dir.c_str(), i);
		if (path.IsDirectory())
			maxLevel = i;
	}
	if (maxLevel < 0)
		return false; // not exist file

	// root directory
	StrPath path;
	path.Format("%s\\%d", s_dir.c_str(), cQTreeGraph::DEFAULT_TREE_LEVEL);

	// collect root node file
	list<string> files;
	list<string> exts;
	exts.push_back(".opath");
	common::CollectFiles(exts, path.c_str(), files);

	// Quad Tree Traverse Stack Memory
	struct sData
	{
		int level;
		int xloc;
		int yloc;
	};
	const int STACK_SIZE = 512;
	sData *stack = new sData[STACK_SIZE];

	int sp = 0;
	for (auto &fileName : files)
	{
		// get xLoc, yLoc from filename
		int xloc, yloc;
		sscanf(StrPath(fileName).GetFileName(), "%04d_%04d.opath", &yloc, &xloc);
		stack[sp++] = { DEFAULT_TREE_LEVEL, xloc, yloc };
	}

	while (sp > 0)
	{
		const int level = stack[sp - 1].level;
		const int xloc = stack[sp - 1].xloc;
		const int yloc = stack[sp - 1].yloc;
		--sp;

		bool isLeaf = false;
		const StrPath fileName = GetNodeFilePath(level, xloc, yloc);
		if (fileName.IsFileExist())
		{
			if (!FindNode(level, xloc, yloc))
				FindAndCreateFromFile(level, xloc, yloc);
			isLeaf = level != DEFAULT_TREE_LEVEL;
		}

		if (!isLeaf && (level < maxLevel))
		{
			int locs[] = { 0,0, 1,0, 0,1, 1,1 }; //x,y loc
			for (int i = 0; i < 4; ++i)
			{
				const int nxloc = (xloc << 1) + locs[i * 2];
				const int nyloc = (yloc << 1) + locs[i * 2 + 1];
				const int nlv = level + 1;
				stack[sp++] = { nlv, nxloc, nyloc };
			}
		}
	}

	SAFE_DELETEA(stack);
	return true;
}


// write quadtree graph data
bool cQTreeGraph::WriteFile()
{
	// make optimize directory
	if (!s_dir.IsFileExist())
		_mkdir(s_dir.c_str());

	// Quad Tree Traverse Stack Memory
	struct sData
	{
		cQuadTree<sNode> *qtree;
		sQuadTreeNode<sNode> *node;
	};
	const int STACK_SIZE = 512;
	sData *stack = new sData[STACK_SIZE];

	int sp = 0;
	for (auto &kv : m_roots)
	{
		cQuadTree<sNode> *qtree = kv.second;
		for (auto &node : qtree->m_roots)
			stack[sp++] = { qtree, node };
	}

	while (sp > 0)
	{
		cQuadTree<sNode> *qtree = stack[sp - 1].qtree;
		sQuadTreeNode<sNode> *node = stack[sp - 1].node;
		--sp;

		// leaf node or root?
		if (!node->children[0] || (node->level == DEFAULT_TREE_LEVEL))
		{
			WriteNode(node);
		}
		
		if (node->children[0])
		{
			// remove *.opath file
			// no need optimize path file if exist children
			// except root level, because fast tree traverse
			if (node->level != DEFAULT_TREE_LEVEL)
			{
				const StrPath fileName = 
					GetNodeFilePath(node->level, node->xLoc, node->yLoc);
				if (fileName.IsFileExist())
					remove(fileName.c_str());
			}

			for (int i = 0; i < 4; ++i)
			{
				if (node->children[i])
				{
					stack[sp].qtree = qtree;
					stack[sp].node = node->children[i];
					++sp;
				}
			}
		}
	}

	SAFE_DELETEA(stack);
	return true;
}


// return node file path
StrPath cQTreeGraph::GetNodeFilePath(
	const int level, const int xLoc, const int yLoc)
{
	StrPath fileName;
	fileName.Format("%s\\%d\\%04d\\%04d_%04d.opath", s_dir.c_str()
		, level, yLoc, yLoc, xLoc);
	return fileName;
}


// read node data
bool cQTreeGraph::ReadNode(sQuadTreeNode<sNode> *node)
{
	if (!node->data.vertices.empty())
		return true; // already loading

	const StrPath fileName = GetNodeFilePath(node->level, node->xLoc, node->yLoc);

	// *.opath file format
	// header, OPAT (opath, optimize path) (4byte)
	// node level (4byte)
	// node xLoc (4byte)
	// node yLoc (4byte)
	// vertex size (4byte)
	// vertex pos (12 byte)
	// vertex transition size (4 byte)
	// loop transition size
	//	- transition to index + flag (4byte)
	//	- if external connect transition (flag -> 0xFFFFFFFF)
	//		- transition to id (8byte)
	std::ifstream ifs(fileName.c_str(), std::ios::binary);
	if (!ifs.is_open())
		return false;

	char format[4];
	ifs.read(format, 4);
	if ((format[0] != 'O')
		|| (format[1] != 'P')
		|| (format[2] != 'A')
		|| (format[3] != 'T'))
		return false;

	int ival = 0;
	ifs.read((char*)&ival, sizeof(ival));
	if (ival != node->level)
		return false;
	ifs.read((char*)&ival, sizeof(ival));
	if (ival != node->xLoc)
		return false;
	ifs.read((char*)&ival, sizeof(ival));
	if (ival != node->yLoc)
		return false;

	int verticeSize = 0;
	ifs.read((char*)&verticeSize, sizeof(verticeSize));
	if (verticeSize > 0)
		node->data.vertices.resize(verticeSize);

	for (int i = 0; i < verticeSize; ++i)
	{
		sVertex &vtx = node->data.vertices[i];

		ifs.read((char*)&vtx.pos, sizeof(vtx.pos));

		ifs.read((char*)&vtx.trCnt, sizeof(vtx.trCnt));
		if (vtx.trCnt > 0)
		{
			vtx.trs = new sTransition[vtx.trCnt];
			vtx.trCapa = vtx.trCnt;
		}

		for (uint k = 0; k < vtx.trCnt; ++k)
		{
			ifs.read((char*)&ival, sizeof(ival)); // to index
			if (ival == 0xFFFFFFFF)
			{
				// external connect transition
				ifs.read((char*)&vtx.trs[k].to, sizeof(vtx.trs[k].to));
			}
			else
			{
				MAKE_QGID(vtx.trs[k].to
					, ival, node->level, node->xLoc, node->yLoc);
			}
		}
	}

	return true;
}


// write node data
bool cQTreeGraph::WriteNode(sQuadTreeNode<sNode> *node)
{
	const StrPath fileName = GetNodeFilePath(node->level, node->xLoc, node->yLoc);

	// check directory
	{
		StrPath dirPath;
		dirPath.Format("%s\\%d", s_dir.c_str(), node->level);
		if (!common::IsFileExist(dirPath))
			_mkdir(dirPath.c_str());

		dirPath.Format("%s\\%d\\%04d", s_dir.c_str(), node->level, node->yLoc);
		if (!common::IsFileExist(dirPath))
			_mkdir(dirPath.c_str());
	}

	// *.opath file format
	// header, OPAT (opath, optimize path) (4byte)
	// node level (4byte)
	// node xLoc (4byte)
	// node yLoc (4byte)
	// vertex size (4byte)
	// vertex position (12 byte)
	// vertex transition size (4 byte)
	// loop transition size
	//	- transition to index + flag (4byte)
	//	- if external connect transition (flag -> 0xFFFFFFFF)
	//		- transition to id (8byte)
	std::ofstream ofs(fileName.c_str(), std::ios::binary);
	if (!ofs.is_open())
		return false;

	ofs.write("OPATH", 4);
	ofs.write((char*)&node->level, sizeof(node->level));
	ofs.write((char*)&node->xLoc, sizeof(node->xLoc));
	ofs.write((char*)&node->yLoc, sizeof(node->yLoc));
	int ival = (int)node->data.vertices.size();
	ofs.write((char*)&ival, sizeof(ival));

	qgid nodeId;
	MAKE_QGID(nodeId, 0, node->level, node->xLoc, node->yLoc);

	for (uint i=0; i < node->data.vertices.size(); ++i)
	{
		auto &vtx = node->data.vertices[i];
		ofs.write((char*)&vtx.pos, sizeof(vtx.pos));
		ofs.write((char*)&vtx.trCnt, sizeof(vtx.trCnt));
		for (uint i = 0; i < vtx.trCnt; ++i)
		{
			if (COMPARE_QID(nodeId, vtx.trs[i].to))
			{
				int idx;
				PARSE_QGID_INDEX(vtx.trs[i].to, idx);
				ofs.write((char*)&idx, sizeof(idx));
			}
			else
			{
				// external connect transition
				ival = 0xFFFFFFFF;
				ofs.write((char*)&ival, sizeof(ival));
				ofs.write((char*)&vtx.trs[i].to, sizeof(vtx.trs[i].to));
			}
		}
	}
	return true;
}


// add point
// pos: relation pos
qgid cQTreeGraph::AddPoint(const Vector3 &pos
	, map<qgid, qgid> &mapping
	, const bool isAverage //= true
)
{
	const Vector3 gpos = cQuadTree<>::GetGlobalPos(pos);
	const sRectf rect = sRectf::Rect(gpos.x, gpos.z, 0, 0);
	const auto result = cQuadTree<>::GetNodeLevel(rect);
	const int level = std::get<0>(result);
	const int xLoc = std::get<1>(result);
	const int yLoc = std::get<2>(result);

	FindBestNode(level, xLoc, yLoc); // load node file
	cQuadTree<sNode> *qtree = FindRootTree(level, xLoc, yLoc);
	if (!qtree)
		return 0; // error occurred!!

	m_isDivide = false;
	return AddPointInBestNode(qtree, rect, pos, mapping, isAverage);
}


// find quadtree from lv, xloc, yloc
// if not exist, create quadtree
cQuadTree<sNode>* cQTreeGraph::FindAndCreateRootTree(const int level
	, const int xLoc, const int yLoc)
{
	if (level < DEFAULT_TREE_LEVEL)
		return nullptr; // error occurred!!

	int lv = level;
	int x = xLoc;
	int y = yLoc;
	while (lv > DEFAULT_TREE_LEVEL)
	{
		--lv;
		x >>= 1;
		y >>= 1;
	}
	const uint64 key = cQuadTree<>::MakeKey(lv, x, y);
	auto it = m_roots.find(key);
	if (m_roots.end() != it)
		return it->second;

	cQuadTree<sNode> *newQtree = new cQuadTree<sNode>();
	m_roots[key] = newQtree;

	sQuadTreeNode<sNode> *root = new sQuadTreeNode<sNode>();
	root->level = lv;
	root->xLoc = x;
	root->yLoc = y;
	newQtree->m_rootRect = cQuadTree<sNode>::GetNodeRect(lv, x, y);
	newQtree->Insert(root);
	newQtree->m_roots.push_back(root); // tricky code

	FindAndCreateFromFile(lv, x, y);

	return newQtree;
}


// find root tree, level:DEFAULT_TREE_LEVEL
cQuadTree<sNode>* cQTreeGraph::FindRootTree(const int level
	, const int xLoc, const int yLoc)
{
	if (level > DEFAULT_TREE_LEVEL)
	{
		int lv = level;
		int x = xLoc;
		int y = yLoc;
		while (lv != DEFAULT_TREE_LEVEL)
		{
			--lv;
			x >>= 1;
			y >>= 1;
		}
		const uint64 key = cQuadTree<>::MakeKey(lv, x, y);
		auto it = m_roots.find(key);
		if (m_roots.end() != it)
			return it->second;
	}
	else if (level == DEFAULT_TREE_LEVEL)
	{
		const uint64 key = cQuadTree<>::MakeKey(level, xLoc, yLoc);
		auto it = m_roots.find(key);
		if (m_roots.end() != it)
			return it->second;
	}
	return nullptr;
}


// find best fit node
// and create node from file
sQuadTreeNode<sNode>* cQTreeGraph::FindBestNode(const int level
	, const int xLoc, const int yLoc)
{
	cQuadTree<sNode> *qtree = FindAndCreateRootTree(level, xLoc, yLoc);
	if (!qtree)
		return nullptr; // error occurred!!

	int lv = level;
	int x = xLoc;
	int y = yLoc;
	while (lv > DEFAULT_TREE_LEVEL)
	{
		const uint64 key = cQuadTree<>::MakeKey(lv, x, y);
		sQuadTreeNode<sNode> *node = qtree->GetNode(key);
		if (node)
			return node;

		node = FindAndCreateFromFile(lv, x, y);
		if (node)
			return node;

		--lv;
		x >>= 1;
		y >>= 1;
	}

	return qtree->m_roots[0];
}


// find node from file
// if exist, read file and create node
// if not exist parent and sibling node, create and read from file
// bottom up creation
sQuadTreeNode<sNode>* cQTreeGraph::FindAndCreateFromFile(const int level
	, const int xLoc, const int yLoc)
{
	const StrPath fileName = GetNodeFilePath(level, xLoc, yLoc);
	if (!fileName.IsFileExist())
		return nullptr;

	cQuadTree<sNode> *qtree = FindAndCreateRootTree(level, xLoc, yLoc);
	if (!qtree)
		return nullptr;

	if (level == DEFAULT_TREE_LEVEL)
	{
		ReadNode(qtree->m_roots[0]);
		return qtree->m_roots[0]; // finish
	}

	sQuadTreeNode<sNode> *child = new sQuadTreeNode<sNode>();
	child->level = level;
	child->xLoc = xLoc;
	child->yLoc = yLoc;
	if (!ReadNode(child))
	{
		delete child;
		return nullptr;
	}
	sQuadTreeNode<sNode> *retVal = child;

	// check parent and sibling node
	int clv = level; // child level
	int cx = xLoc; // child xLoc
	int cy = yLoc; // child yLoc
	int lv = level;
	int x = xLoc;
	int y = yLoc;
	while (lv > DEFAULT_TREE_LEVEL)
	{
		--lv;
		x >>= 1;
		y >>= 1;

		sQuadTreeNode<sNode> *parent = FindNode(lv, x, y);
		const bool hasParent = parent ? true : false;
		if (!parent)
		{
			// create node and read from file
			parent = new sQuadTreeNode<sNode>();
			parent->level = lv;
			parent->xLoc = x;
			parent->yLoc = y;
			ReadNode(parent); // file not exist? no problem
		}

		// create children
		if (parent->children[0] != nullptr)
			break; // error occurred!!, already exist children

		sQuadTreeNode<sNode> *children[4];
		int locs[] = { 0,0, 1,0, 0,1, 1,1 }; //x,y loc
		for (int i = 0; i < 4; ++i)
		{
			const int nlv = lv + 1;
			const int nx = (x << 1) + locs[i * 2];
			const int ny = (y << 1) + locs[i * 2 + 1];
			if ((clv == nlv) && (cx == nx) && (cy == ny))
			{
				children[i] = child;
				continue;
			}
			sQuadTreeNode<sNode> *node = new sQuadTreeNode<sNode>();
			node->level = nlv;
			node->xLoc = nx;
			node->yLoc = ny;
			ReadNode(node);
			children[i] = node;
		}

		qtree->InsertChildren(parent, children);

		if (hasParent)
			break; // finish

		clv = lv;
		cx = x;
		cy = y;
		child = parent;
	}

	return retVal;
}


// find node
sQuadTreeNode<sNode>* cQTreeGraph::FindNode(const qgid id)
{
	int index, level, xLoc, yLoc;
	PARSE_QGID(id, index, level, xLoc, yLoc);
	return FindNode(level, xLoc, yLoc);
}


// find node
sQuadTreeNode<sNode>* cQTreeGraph::FindNode(const int level
	, const int xLoc, const int yLoc)
{
	cQuadTree<sNode> *qtree = FindRootTree(level, xLoc, yLoc);
	if (!qtree)
		return nullptr;
	return qtree->GetNode(level, xLoc, yLoc);
}


// find node correspond position
// pos: local pos
sQuadTreeNode<sNode>* cQTreeGraph::FindBestNode(const Vector3 &pos)
{
	const Vector3 gpos = cQuadTree<>::GetGlobalPos(pos);
	const sRectf rect = sRectf::Rect(gpos.x, gpos.z, 0, 0);
	const auto result = cQuadTree<>::GetNodeLevel(rect);
	const int level = std::get<0>(result);
	const int xLoc = std::get<1>(result);
	const int yLoc = std::get<2>(result);
	if (xLoc < 0)
		return nullptr; // error occurred!!
	return FindBestNode(level, xLoc, yLoc);
}


// get vertex position
Vector3 cQTreeGraph::GetVertexPos(const qgid id)
{
	int index, level, xloc, yloc;
	PARSE_QGID(id, index, level, xloc, yloc);

	sQuadTreeNode<sNode> *node = FindNode(id);

	if (!node)
	{
		node = FindAndCreateFromFile(level, xloc, yloc);
		if (!node)
			return Vector3(); // error occurred!!
	}
	
	return node->data.vertices[index].pos;
}


// add point in best quadtree node
// pos: relation pos
qgid cQTreeGraph::AddPointInBestNode(cQuadTree<sNode> *qtree
	, const sRectf &rect, const Vector3 &pos
	, map<qgid, qgid> &mapping
	, const bool isAverage //= true
)
{
	//m_mappingIds.clear();

	sQuadTreeNode<sNode> *rootNode = qtree->m_roots[0];
	int level = rootNode->level;
	int xLoc = rootNode->xLoc;
	int yLoc = rootNode->yLoc;
	qgid ret = 0;

	// find best tree node
	sQuadTreeNode<sNode> *node = qtree->m_roots[0];
	while (1)
	{
		const bool hasChild = node->children[0] != nullptr;
		if ((hasChild || (node->data.vertices.size() > MAX_TABLESIZE))
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
				if (child->data.vertices.size() > MAX_TABLESIZE)
					continue; // goto children
				if (child->children[0] != nullptr)
					continue; // goto children
			}
			else
			{
				// make child node and table
				DivideNodeToChild(qtree, node, mapping);

				node = qtree->GetNode(level, x, y);
				continue; // goto children
			}
		}
		if (!node)
			break; // error occurred!!

		// find duplicate pos
		bool isFind = false;
		if (isAverage)
		{
			float minDist = FLT_MAX;
			for (uint i=0; i < node->data.vertices.size(); ++i)
			{
				sVertex &vtx = node->data.vertices[i];
				const float dist = pos.Distance(vtx.pos);

				// same position?
				if (dist < 0.1f)
				{
					// average position, update
					vtx.pos = ((vtx.pos * (float)vtx.accCnt) / (float)(vtx.accCnt + 1))
						+ (pos / (float)(vtx.accCnt + 1));
					vtx.accCnt += 1;

					qgid id = MovePoint(node, i, mapping);
					if (id == 0) // no move?
						MAKE_QGID(id, (int)i, node->level, node->xLoc, node->yLoc);
					isFind = true;

					if (minDist > dist)
					{
						minDist = dist;
						ret = id;
					}
				}
			}
		}

		// no near position, add new pos
		if (!isFind)
		{
			sVertex vtx;
			InitVertex(vtx, pos, 1);
			node->data.vertices.push_back(vtx);

			//node->data.table.push_back({ 1, pos });
			MAKE_QGID(ret, (int)node->data.vertices.size() - 1
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
	, sQuadTreeNode<sNode> *node
	, map<qgid, qgid> &mapping)
{
	m_isDivide = true;
	qtree->InsertChildren(node);

	map<qgid, qgid> ids; // key: old id, value: new id

	const int nextLv = node->level + 1;

	// divide graph
	for (uint i = 0; i < node->data.vertices.size(); ++i)
	{
		auto &vtx = node->data.vertices[i];
		const Vector3 gp = cQuadTree<>::GetGlobalPos(vtx.pos);
		const sRectf r = sRectf::Rect(gp.x, gp.z, 0, 0);
		const auto result = cQuadTree<>::GetNodeLocation(r, nextLv);
		const int xLoc = std::get<0>(result);
		const int yLoc = std::get<1>(result);

		sQuadTreeNode<sNode> *pn = qtree->GetNode(nextLv, xLoc, yLoc);
		if (!pn)
		{
			dbg::ErrLogp("DivideNodeToChild, not found nextLv node");
			return false; // error occurred!!
		}

		pn->data.vertices.push_back(vtx);

		const int newIdx = pn->data.vertices.size() - 1;

		qgid id0, id1;
		MAKE_QGID(id0, i, node->level, node->xLoc, node->yLoc);
		MAKE_QGID(id1, newIdx, nextLv, xLoc, yLoc);
		ids[id0] = id1;
	}

	qgid nodeId;
	MAKE_QGID(nodeId, 0, node->level, node->xLoc, node->yLoc);

	// update transition id
	set<qgid> toIds;
	for (uint i = 0; i < node->data.vertices.size(); ++i)
	{
		qgid oldId;
		MAKE_QGID(oldId, i, node->level, node->xLoc, node->yLoc);

		auto it = ids.find(oldId);
		if (ids.end() == it)
			continue; // error occurred!!

		const qgid newId = it->second;
		int index, level, xLoc, yLoc;
		PARSE_QGID(newId, index, level, xLoc, yLoc);
		sQuadTreeNode<sNode> *pn = qtree->GetNode(level, xLoc, yLoc);
		if (!pn)
			return false; // error occurred!!

		sVertex &newVtx = pn->data.vertices[index];
		
		for (uint k = 0; k < newVtx.trCnt; ++k)
		{
			sTransition &tr = newVtx.trs[k];

			int idx1, lv1, x1, y1;
			PARSE_QGID(tr.to, idx1, lv1, x1, y1);

			if (COMPARE_QID(tr.to, nodeId))
			{
				// same quad tree node, update id
				auto it0 = ids.find(tr.to);
				if (ids.end() == it0)
					continue; // error occurred!!
				tr.to = it0->second;
			}
			else
			{
				toIds.insert(tr.to);
			}
		}

		auto &vtx = node->data.vertices[i];
		vtx.trCnt = 0;
		vtx.trs = nullptr; // memory move, so null
	}

	// update external connection vertex transition
	for (auto to : toIds)
	{
		int idx1, lv1, x1, y1;
		PARSE_QGID(to, idx1, lv1, x1, y1);

		sVertex *toVtx = FindVertex(to);
		if (!toVtx)
		{
			FindAndCreateFromFile(lv1, x1, y1);
			toVtx = FindVertex(to);
			if (!toVtx)
			{
				dbg::ErrLogp("DivideNodeToChild, not found tr.to \n");
				continue; // error occurred!!
			}
		}

		for (uint m = 0; m < toVtx->trCnt; ++m)
		{
			const qgid toto = toVtx->trs[m].to;
			// pointed to this node?
			if (COMPARE_QID(toto, nodeId))
			{
				auto it = ids.find(toto);
				if (ids.end() == it)
					continue; // error occurred!!
				toVtx->trs[m].to = it->second;
			}
		}
	}

	// clear current node data
	node->data.vertices.clear();
	node->data.vertices.shrink_to_fit(); // clear memory

	if (mapping.empty())
	{
		mapping = ids;
	}
	else
	{
		// multiple link
		for (auto &kv : mapping)
		{
			auto it = ids.find(kv.second);
			if (ids.end() != it)
				kv.second = it->second;
		}
		for (auto &kv : ids)
		{
			auto it = mapping.find(kv.first);
			if (mapping.end() == it)
				mapping[kv.first] = kv.second;
		}
	}
	return true;
}


// move fromNode table point to another node
// check table[index] poition move another node
// return moving point qgid, no move return 0
qgid cQTreeGraph::MovePoint(sQuadTreeNode<sNode> *fromNode, const uint index
	, map<qgid, qgid> &mapping
	, const bool isCalcAverage //= false
)
{
	const Vector3 pos = fromNode->data.vertices[index].pos;
	const Vector3 gpos = cQuadTree<>::GetGlobalPos(pos);
	const sRectf rect = sRectf::Rect(gpos.x, gpos.z, 0, 0);
	const auto result = cQuadTree<>::GetNodeLocation(rect, fromNode->level);
	int xLoc = std::get<0>(result);
	int yLoc = std::get<1>(result);
	if (fromNode->xLoc == xLoc && fromNode->yLoc == yLoc)
		return 0; // same node? finish

	qgid oldId;
	MAKE_QGID(oldId, index, fromNode->level, fromNode->xLoc, fromNode->yLoc);

	// no average: continue divide problem
	const qgid newId = AddPoint(pos, mapping, isCalcAverage);

	map<qgid, qgid> ids;
	ids[oldId] = newId;

	set<qgid> toIds;
	for (uint i = 0; i < fromNode->data.vertices.size(); ++i)
	{
		sVertex &vtx = fromNode->data.vertices[i];
		for (uint k = 0; k < vtx.trCnt; ++k)
		{
			sTransition &tr = vtx.trs[k];
			int idx1, lv1, x1, y1;
			PARSE_QGID(tr.to, idx1, lv1, x1, y1);

			if (COMPARE_QID(tr.to, oldId))
			{
				// same node
				if (tr.to == oldId)
					tr.to = newId;
				else if ((uint)idx1 > index)
					MAKE_QGID(tr.to, idx1 - 1, lv1, x1, y1);
			}
			else
			{
				toIds.insert(tr.to);
			}
		}

		if (i > index)
		{
			qgid id0, id1;
			MAKE_QGID(id0, i, fromNode->level, fromNode->xLoc, fromNode->yLoc);
			MAKE_QGID(id1, i - 1, fromNode->level, fromNode->xLoc, fromNode->yLoc);
			ids[id0] = id1;
		}
	}

	// external connection vertex
	for (qgid to : toIds)
	{
		int idx1, lv1, x1, y1;
		PARSE_QGID(to, idx1, lv1, x1, y1);

		sVertex *toVtx = FindVertex(to);
		if (!toVtx)
		{
			FindAndCreateFromFile(lv1, x1, y1);
			toVtx = FindVertex(to);
			if (!toVtx)
			{
				dbg::ErrLogp("MovePoint, not found2 tr.to \n");
				continue; // error occurred!!
			}
		}

		for (uint m = 0; m < toVtx->trCnt; ++m)
		{
			const qgid toto = toVtx->trs[m].to;
			if (COMPARE_QID(toto, oldId))
			{
				int idx2, lv2, x2, y2;
				PARSE_QGID(toto, idx2, lv2, x2, y2);

				if (toto == oldId)
					toVtx->trs[m].to = newId;
				else if ((uint)idx2 > index)
					MAKE_QGID(toVtx->trs[m].to, idx2 - 1, lv2, x2, y2);
			}
		}
	}

	// copy transition to new vertex
	sVertex *newVtx = FindVertex(newId);
	if (newVtx)
	{
		sVertex &curVtx = fromNode->data.vertices[index];
		if (curVtx.trCnt + newVtx->trCnt > newVtx->trCapa)
		{
			const int capa = (curVtx.trCnt + newVtx->trCnt) * 2;
			sTransition *trs = new sTransition[capa];
			memcpy(trs, newVtx->trs, sizeof(sTransition) * newVtx->trCnt);
			newVtx->trCapa = capa;
			SAFE_DELETEA(newVtx->trs);
			newVtx->trs = trs;
		}

		for (uint i = 0; i < curVtx.trCnt; ++i)
		{
			if (FindTransition(newVtx, curVtx.trs[i].to))
				continue; // already exist
			newVtx->trs[newVtx->trCnt++] = curVtx.trs[i];
		}

		curVtx.trCnt = 0;
		SAFE_DELETEA(curVtx.trs);
	}

	common::rotatepopvector(fromNode->data.vertices, index);

	if (mapping.empty())
	{
		mapping = ids;
	}
	else
	{
		// multiple link
		for (auto &kv : mapping)
		{
			auto it = ids.find(kv.second);
			if (ids.end() != it)
				kv.second = it->second;
		}
		for (auto &kv : ids)
		{
			auto it = mapping.find(kv.first);
			if (mapping.end() == it)
				mapping[kv.first] = kv.second;
		}
	}

	return newId;
}


// calculate vertex layout
bool cQTreeGraph::CalcVertexLayout(sQuadTreeNode<sNode> *node
	, map<qgid, qgid> &mapping)
{
	for (uint i = 0; i < node->data.vertices.size();)
	{
		const qgid id = MovePoint(node, i, mapping, true);
		if (id == 0)
			++i;
	}
	return true;
}


// add transition
// id0 : position id0 
// id1 : position id1
bool cQTreeGraph::AddTransition(const qgid id0, const qgid id1)
{
	if (id0 == id1)
		return true; // same point

	sQuadTreeNode<sNode> *node0 = FindNode(id0);
	sQuadTreeNode<sNode> *node1 = FindNode(id1);
	if (!node0 || !node1)
		return false; // error occurred!!

	// check transition length
	{
		int idx0, idx1;
		PARSE_QGID_INDEX(id0, idx0);
		PARSE_QGID_INDEX(id1, idx1);
		const float len = node0->data.vertices[idx0].pos.Distance(
			node1->data.vertices[idx1].pos
		);
		if (len > 100.0f)
			return true; // error, too long
	}

	sVertex *vtx0 = FindVertex(node0, id0);
	sVertex *vtx1 = FindVertex(node1, id1);
	if (!vtx0 || !vtx1)
		return false; // error occurred!!

	// add bidirection
	sTransition tr0;
	tr0.to = id1;
	sTransition *ptr0 = FindTransition(vtx0, id1);
	if (!ptr0)
		AddVertexTransition(vtx0, tr0);

	sTransition tr1;
	tr1.to = id0;
	sTransition *ptr1 = FindTransition(vtx1, id0);
	if (!ptr1)
		AddVertexTransition(vtx1, tr1);

	return true;
}


// find near edge
// distance : max distance
// return : near edge
//			no near edge : {0,0}
sEdge cQTreeGraph::FindNearEdge(const Vector3 &pos
	, const float distance
	, const qgid exceptId //= 0
)
{
	const Vector3 gpos = cQuadTree<>::GetGlobalPos(pos);
	const sRectf rect = sRectf::Rect(gpos.x, gpos.z, 0, 0);
	const auto result = cQuadTree<>::GetNodeLevel(rect);
	int level = std::get<0>(result);
	int xLoc = std::get<1>(result);
	int yLoc = std::get<2>(result);
	if (xLoc < 0)
		return {0,0}; // error occurred!!

	sQuadTreeNode<sNode> *node = FindBestNode(level, xLoc, yLoc);
	if (!node)
		return {0,0}; // error occurred!!

	qgid nodeId;
	MAKE_QGID(nodeId, 0, node->level, node->xLoc, node->yLoc);

	float minLen = FLT_MAX;
	sEdge edge;
	for (auto &vtx : node->data.vertices)
	{
		const Vector3 &p0 = vtx.pos;

		for (uint i = 0; i < vtx.trCnt; ++i)
		{
			const sTransition &tr = vtx.trs[i];
			if (tr.to == exceptId)
				continue;

			Vector3 p1;
			if (COMPARE_QID(nodeId, tr.to))
			{
				int idx1;
				PARSE_QGID_INDEX(tr.to, idx1);
				p1 = node->data.vertices[idx1].pos;
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
				qgid id;
				MAKE_QGID(id, 0, node->level, node->xLoc, node->yLoc);
				edge = sEdge(id, tr.to);
			}
		}
	}

	return edge;
}


// merge path
bool cQTreeGraph::MergePath(cPathList &pathList, const float distance)
{
	vector<Vector3> path;
	path.reserve(8);

	vector<set<sEdge>> allAdjPath; // all adjacent path

	while (!pathList.IsEnd())
	{
		auto res = pathList.GetNextPath();
		const Vector3 p0 = std::get<0>(res);
		const Vector3 p1 = std::get<1>(res);
		if (p0 == Vector3::Zeroes && p1 == Vector3::Zeroes)
			break; // finish

		if (p0.Distance(p1) > 100.f)
			break; // too distance

		path.push_back(p0);
		path.push_back(p1);

		// find near vertex
		sQuadTreeNode<sNode> *node0 = FindBestNode(p0);
		if (!node0)
			return false; // error occurred!!

		qgid nodeId;
		MAKE_QGID(nodeId, 0, node0->level, node0->xLoc, node0->yLoc);

		Line line0(p0, p1);

		set<sEdge> adjPath; // ajacent path
		for (uint i = 0; i < node0->data.vertices.size(); ++i)
		{
			sVertex *vtx0 = &node0->data.vertices[i];
			for (uint k = 0; k < vtx0->trCnt; ++k)
			{
				sTransition &tr = vtx0->trs[k];

				int idx1, lv1, x1, y1;
				PARSE_QGID(tr.to, idx1, lv1, x1, y1);

				sVertex *vtx1 = nullptr;
				if (COMPARE_QID(tr.to, nodeId))
				{
					vtx1 = &node0->data.vertices[idx1];
				}
				else
				{
					vtx1 = FindVertex(tr.to);
					if (!vtx1)
					{
						FindAndCreateFromFile(lv1, x1, y1);
						vtx1 = FindVertex(tr.to);
						if (!vtx1)
						{
							dbg::ErrLogp("MergePath, not found tr.to \n");
							continue; // error occurred!!
						}
					}
				}
				if (!vtx1)
					continue; // error occurred!!

				Line line1(vtx0->pos, vtx1->pos);

				// check line direction, position
				const float dot = line0.dir.DotProduct(line1.dir);
				if (abs(dot) < 0.9f)
					continue; // no matching

				if ((line0.GetDistance(vtx0->pos) < distance)
					|| (line0.GetDistance(vtx1->pos) < distance)
					|| (line1.GetDistance(p0) < distance)
					|| (line1.GetDistance(p1) < distance)
					)
				{
					const Vector3 c0 = (line1.dir * line1.len) + line1.pos 
						+ (line0.dir * line0.len);
					const Vector3 c1 = (line1.dir * -line1.len) + line1.pos
						+ (line1.dir * -line0.len);
					if ((c0.Distance(line0.pos) < (distance * 0.3f))
						|| (c1.Distance(line0.pos) < (distance * 0.3f))
						)
					{
						// this adjacent path is previouse path
						// no overlay path
						continue;
					}

					// find adjacent path
					qgid id0;
					MAKE_QGID(id0, i, node0->level, node0->xLoc, node0->yLoc);
					if ((adjPath.end() == adjPath.find(sEdge(id0, tr.to)))
						&& (adjPath.end() == adjPath.find(sEdge(tr.to, id0))))
					{
						adjPath.insert(sEdge(id0, tr.to));
					}
				}
			}
		}
		allAdjPath.push_back(adjPath);

		if (adjPath.empty())
			break; // no more near path
	}

	// merge
	set<sEdge> checks;
	set<sQuadTreeNode<sNode>*> calcNodes;
	sEdge lastEdge(0, 0);
	for (uint i = 1; i < path.size(); i += 2)
	{
		const Vector3 pos0 = path[i - 1];
		const Vector3 pos1 = path[i];

		set<sEdge> *adjPath = nullptr;
		if (allAdjPath.size() > (i / 2))
			adjPath = &allAdjPath[i / 2];

		if (adjPath && !adjPath->empty())
		{
			// is exist adjcent path
			// average position
			for (auto &edge : *adjPath)
			{
				// check already calcpath
				if (checks.end() != checks.find(edge))
					continue;
				checks.insert(edge);
				checks.insert(sEdge(edge.to, edge.from));

				sVertex *vtx0 = FindVertex(edge.from);
				sVertex *vtx1 = FindVertex(edge.to);
				if (!vtx0 || !vtx1)
					continue; // error occurred!!

				vtx0->accCnt++;
				vtx1->accCnt++;
				const int cnt = (vtx0->accCnt + vtx1->accCnt) / 2;

				Line line0(vtx0->pos, vtx1->pos);
				Line line1(pos0, pos1);
				if (line0.dir.DotProduct(line1.dir) < 0.f)
					line1.dir *= -1.f;

				// average direction
				line0.dir = (line0.dir + line1.dir).Normal();
				const Vector3 right = Vector3(0, 1, 0).CrossProduct(line0.dir).Normal();
				const Vector3 diff = line1.pos - line0.pos;
				const float dist = right.DotProduct(diff);
				const float moveLen = (dist / cnt);// *(isRight ? -1.0f : 1.0f);
				line0.pos += right * moveLen;
				vtx0->pos = line0.dir * -line0.len + line0.pos;
				vtx1->pos = line0.dir * line0.len + line0.pos;
				lastEdge = edge;

				// calculate vertex layout node
				sQuadTreeNode<sNode> *node0 = FindNode(edge.from);
				sQuadTreeNode<sNode> *node1 = FindNode(edge.to);
				if (node0)
					calcNodes.insert(node0);
				if (node1)
					calcNodes.insert(node1);
			}
		}
		else
		{
			map<qgid, qgid> mapping;
			if (!calcNodes.empty())
				for (auto node : calcNodes)
					CalcVertexLayout(node, mapping);

			qgid id0 = AddPoint(pos0, mapping);

			// connect last adacent edge
			if (lastEdge != sEdge(0, 0))
			{
				if (!mapping.empty())
				{
					auto it = mapping.find(lastEdge.from);
					if (mapping.end() != it)
						lastEdge.from = it->second;
					it = mapping.find(lastEdge.to);
					if (mapping.end() != it)
						lastEdge.to = it->second;
				}

				sVertex *vtx0 = FindVertex(lastEdge.from);
				sVertex *vtx1 = FindVertex(lastEdge.to);
				if (vtx0 && vtx1)
				{
					if (vtx0->pos.Distance(pos0) < vtx1->pos.Distance(pos0))
					{
						AddTransition(lastEdge.from, id0);
					}
					else
					{
						AddTransition(lastEdge.to, id0);
					}
				}
			}

			mapping.clear();
			qgid id1 = AddPoint(pos1, mapping);
			if (!mapping.empty()) {
				auto it = mapping.find(id0);
				if (mapping.end() != it)
					id0 = it->second;
			}
			if (id0 == id1)
				continue; // same vertex? next
			AddTransition(id0, id1);
		}
	}

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
	return &node->data.vertices[index];
}


// find vertex
sVertex* cQTreeGraph::FindVertex(sQuadTreeNode<sNode>*node, const qgid id)
{
	int index, level, xLoc, yLoc;
	PARSE_QGID(id, index, level, xLoc, yLoc);
	return &node->data.vertices[index];
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

	int idx0, lv0, x0, y0;
	PARSE_QGID(tr.to, idx0, lv0, x0, y0);
	if (idx0 == 1 && lv0 == 6 && x0 == 547 && y0 == 227)
	{
		int a = 0;
	}

	vtx->trs[vtx->trCnt] = tr;
	vtx->trCnt++;
	return true;
}


// initialize vertex, transition
void cQTreeGraph::InitVertex(sVertex &vtx, const Vector3 &pos, const uint accCnt)
{
	vtx.pos = pos;
	vtx.accCnt = accCnt;
	vtx.trCnt = 0;
	vtx.trCapa = sVertex::DEFAULT_TRANSITION;
	vtx.trs = new sTransition[sVertex::DEFAULT_TRANSITION];
	ZeroMemory(vtx.trs, sizeof(sTransition) * sVertex::DEFAULT_TRANSITION);
}


// generate graph vertex, transition line
bool cQTreeGraph::CreateGraphLines(graphic::cRenderer &renderer
	, sQuadTreeNode<sNode> *node)
{
	using namespace graphic;

	if (node->data.lineList)
		return true; // already exist

	node->data.lineList = new cDbgLineList();

	int maxLine = 0;
	for (auto &vtx : node->data.vertices)
		maxLine += vtx.trCnt;
	if (maxLine == 0)
		return true;

	node->data.lineList->Create(renderer, maxLine, cColor::WHITE);

	qgid nodeId;
	MAKE_QGID(nodeId, 0, node->level, node->xLoc, node->yLoc);

	set<qgid> checks; // duplicate check
	for (uint k=0; k < node->data.vertices.size(); ++k)
	{
		auto &vtx = node->data.vertices[k];
		const Vector3 &p0 = vtx.pos;
		qgid id0;
		MAKE_QGID(id0, k, node->level, node->xLoc, node->yLoc);

		checks.insert(id0);

		for (uint i = 0; i < vtx.trCnt; ++i)
		{
			Vector3 p1;
			const sTransition &tr = vtx.trs[i];
			if (checks.end() != checks.find(tr.to))
				continue; // already connect

			if (COMPARE_QID(nodeId, tr.to))
			{
				int idx11, lv1, x1, y1;
				PARSE_QGID(tr.to, idx11, lv1, x1, y1);

				int idx1;
				PARSE_QGID_INDEX(tr.to, idx1);
				p1 = node->data.vertices[idx1].pos;
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
	for (auto &kv : m_roots)
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
	m_roots.clear();
}
