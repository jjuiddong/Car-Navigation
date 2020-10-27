
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
	if (node->level == 9 && node->xLoc == 4362 && node->yLoc == 1818)
	{
		int a = 0;
	}

	if (node->level == 7 && node->xLoc == 1091 && node->yLoc == 454)
	{
		int a = 0;
	}

	if (!node->data.table.empty())
		return true; // already loading
	if (!node->data.vertices.empty())
		return true; // already loading

	const StrPath fileName = GetNodeFilePath(node->level, node->xLoc, node->yLoc);

	// *.opath file format
	// header, OPAT (opath, optimize path) (4byte)
	// node level (4byte)
	// node xLoc (4byte)
	// node yLoc (4byte)
	// table size (4byte)
	// table (sAccPos * table size)
	// vertex size (4byte)
	// vertex table index (4 byte)
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
	ifs.read((char*)&ival, sizeof(ival)); // table size
	if (ival > 0)
	{
		node->data.table.resize(ival);
		// tricky code, depend on memory alignment size
		ifs.read((char*)&node->data.table[0]
			, sizeof(sAccPos) * node->data.table.size());
	}

	int verticeSize = 0;
	ifs.read((char*)&verticeSize, sizeof(verticeSize));
	if (verticeSize > 0)
		node->data.vertices.resize(verticeSize);

	for (int i = 0; i < verticeSize; ++i)
	{
		sVertex &vtx = node->data.vertices[i];

		ifs.read((char*)&ival, sizeof(ival)); // index
		MAKE_QGID(vtx.id, ival, node->level, node->xLoc, node->yLoc);

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

	if (node->data.table.size() != node->data.vertices.size())
	{
		int a = 0;
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
	// table size (4byte)
	// table (sAccPos * table size)
	// vertex size (4byte)
	// vertex table index (4 byte)
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
	int ival = (int)node->data.table.size();
	ofs.write((char*)&ival, sizeof(ival));
	if (!node->data.table.empty())
	{
		// tricky code, depend on memory alignment size
		ofs.write((char*)&node->data.table[0]
			, sizeof(sAccPos) * node->data.table.size());
	}
	ival = (int)node->data.vertices.size();
	ofs.write((char*)&ival, sizeof(ival));
	for (auto &vtx : node->data.vertices)
	{
		int idx;
		PARSE_QGID_INDEX(vtx.id, idx);
		ofs.write((char*)&idx, sizeof(idx));
		ofs.write((char*)&vtx.trCnt, sizeof(vtx.trCnt));
		for (uint i = 0; i < vtx.trCnt; ++i)
		{
			if (COMPARE_QID(vtx.id, vtx.trs[i].to))
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
	return AddPointInBestNode(qtree, rect, pos, isAverage);
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

		if (lv == 13 && x == 69821 && y == 29040)
		{
			int a = 0;
		}
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

		if (lv == 9 && x == 4362 && y == 1818)
		{
			int a = 0;
		}

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
	if (node && node->children[0])
	{
		int a = 0;
	}

	if (!node)
	{
		node = FindAndCreateFromFile(level, xloc, yloc);
		if (!node)
			return Vector3(); // error occurred!!
	}
	
	if (node->children[0])
	{
		int a = 0;
	}

	//int index;
	//PARSE_QGID_INDEX(id, index);
	return node->data.table[index].pos;
}


// add point in best quadtree node
// pos: relation pos
qgid cQTreeGraph::AddPointInBestNode(cQuadTree<sNode> *qtree
	, const sRectf &rect, const Vector3 &pos
	, const bool isAverage //= true
)
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
		if (isAverage)
		{
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

					ret = MovePoint(node, i);
					if (ret == 0) // no move?
						MAKE_QGID(ret, (int)i, node->level, node->xLoc, node->yLoc);
					isFind = true;
					break;
				}
			}
		}

		// no near position, add new pos
		if (!isFind)
		{
			node->data.table.push_back({ 1, pos });
			MAKE_QGID(ret, (int)node->data.table.size() - 1
				, node->level, node->xLoc, node->yLoc);

			//int i, l, x, y;
			//PARSE_QGID(ret, i, l, x, y);
			//if ((i == 198) && (l == 9) && (x == 4361) && (y == 1814))
			//{
			//	int a = 0;
			//}
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

	if (node->level == 9 && node->xLoc == 4363 && node->yLoc == 1819)
	{
		int a = 0;
	}

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
		{
			dbg::ErrLogp("DivideNodeToChild, not found nextLv node");
			return false; // error occurred!!
		}

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

		int idx0, lv0, x0, y0;
		PARSE_QGID(vtx.id, idx0, lv0, x0, y0);

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
				if (toVtx->id == 120318592067)
				{
					int a = 0;
				}

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


// move fromNode table point to another node
// check table[index] poition move another node
// return moving point qgid, no move return 0
qgid cQTreeGraph::MovePoint(sQuadTreeNode<sNode> *fromNode, const int index)
{
	const Vector3 pos = fromNode->data.table[index].pos;
	const Vector3 gpos = cQuadTree<>::GetGlobalPos(pos);
	const sRectf rect = sRectf::Rect(gpos.x, gpos.z, 0, 0);
	const auto result = cQuadTree<>::GetNodeLocation(rect, fromNode->level);
	int xLoc = std::get<0>(result);
	int yLoc = std::get<1>(result);
	if (fromNode->xLoc == xLoc && fromNode->yLoc == yLoc)
		return 0; // same node? finish

	if (fromNode->data.table.size() != fromNode->data.vertices.size())
	{
		int a = 0;
	}

	if ((fromNode->level == 9) 
		&& (fromNode->xLoc == 4362) && (fromNode->yLoc == 1818))
	{
		int a = 0;
	}

	//if ((fromNode->level == 9)
	//	&& (fromNode->xLoc == 4360) && (fromNode->yLoc == 1814))
	//{
	//	int a = 0;
	//}

	//if ((fromNode->level == 7)
	//	&& (fromNode->xLoc == 1091) && (fromNode->yLoc == 453))
	//{
	//	int a = 0;
	//}

	map<qgid, qgid> ids;

	// no average: continue divide problem
	const qgid newId = AddPoint(pos, false);

	//{
	//	int idx1, lv1, x1, y1;
	//	PARSE_QGID(newId, idx1, lv1, x1, y1);
	//	int a = 0;
	//}

	// remove table
	common::rotatepopvector(fromNode->data.table, index);

	// move vertex
	int rmVtxIdx = -1; // remove vertex index
	for (uint i=0; i < fromNode->data.vertices.size(); ++i)
	{
		sVertex &vtx = fromNode->data.vertices[i];

		int idx0;
		PARSE_QGID_INDEX(vtx.id, idx0);
		if (idx0 == index)
		{
			rmVtxIdx = (int)i;
			ids[vtx.id] = newId;

			// move vertex to another node
			sQuadTreeNode<sNode> *toNode = FindBestNode(pos);
			if (!toNode)
			{
				dbg::ErrLogp("MovePoint, not found toNode \n");
				break; // error occurred!!
			}

			//int cntt = 0;
			//for (uint mm = 0; mm < toNode->data.vertices.size(); ++mm)
			//{
			//	sVertex &vv = toNode->data.vertices[mm];
			//	if (vv.id == newId)
			//	{
			//		int a = 0;
			//	}
			//}
			sVertex *srcVtx = FindVertex(newId);
			if (srcVtx)
			{ // already exist vertex
				// add two vertex transition
				for (uint k = 0; k < srcVtx->trCnt; ++k)
				{
					sTransition &tr = srcVtx->trs[k];
					if (tr.to == vtx.id)
					{
						//tr.to = 0;
					}
				}
				for (uint k = 0; k < vtx.trCnt; ++k)
				{
					sTransition &tr = vtx.trs[k];
					if (tr.to == srcVtx->id)
					{
						//tr.to = 0;
					}
				}
			}
			else
			{
				sVertex newVtx = vtx; // memory move
				UpdateVertexId(fromNode, newVtx, newId, 0, index, 1);

				vtx.trCnt = 0;
				vtx.trs = nullptr; // memory move
				toNode->data.vertices.push_back(newVtx);
			}
		}
		else if (idx0 < index)
		{
			UpdateVertexId(fromNode, vtx, 0, newId, index, 0);
		}
		else if (idx0 > index)
		{
			// change id (change index)
			qgid chId;
			MAKE_QGID(chId, idx0 - 1,
				fromNode->level, fromNode->xLoc, fromNode->yLoc);
			ids[vtx.id] = chId;
			UpdateVertexId(fromNode, vtx, chId, newId, index, 1);
		}
	}

	// remove vertex
	if (rmVtxIdx >= 0)
		common::rotatepopvector(fromNode->data.vertices, rmVtxIdx);

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

	return newId;
}


// update vertex id to newId
// transition to index - 1 if  idx > index
// newId: update vertex id
// toId: update to id if idx == index
// flag = 0: nothing
//		  1: update vertex id
void cQTreeGraph::UpdateVertexId(sQuadTreeNode<sNode> *node
	, sVertex &vtx, const qgid newId, const qgid toId
	, const int index, const int flag)
{
	for (uint k = 0; k < vtx.trCnt; ++k)
	{
		sTransition &tr = vtx.trs[k];

		int idx1, lv1, x1, y1;
		PARSE_QGID(tr.to, idx1, lv1, x1, y1);

		if (node->level == lv1 && node->xLoc == x1 && node->yLoc == y1)
		{
			// same node
			if (idx1 == index)
				tr.to = toId;
			else if (idx1 > index)
				MAKE_QGID(tr.to, idx1 - 1, lv1, x1, y1);

			if (vtx.id == tr.to)
			{
				int a = 0;
			}
		}
		else
		{
			// update connected vertex transition
			if (flag == 1)
			{
				sVertex *toVtx = FindVertex(tr.to);
				if (!toVtx)
				{
					FindAndCreateFromFile(lv1, x1, y1);
					toVtx = FindVertex(tr.to);
					if (!toVtx)
					{
						dbg::ErrLogp("UpdateVertexId, not found2 tr.to \n");
						continue; // error occurred!!
					}
				}

				for (uint m = 0; m < toVtx->trCnt; ++m)
				{
					if (toVtx->trs[m].to == vtx.id)
					{
						toVtx->trs[m].to = newId;

						if (toVtx->id == newId)
						{
							int a = 0;
						}

						break;
					}
				}
			}
		}
	}

	if (flag == 1)
		vtx.id = newId;
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
		return true; // same point

	sQuadTreeNode<sNode> *node0 = FindNode(id0);
	sQuadTreeNode<sNode> *node1 = FindNode(id1);
	if (!node0 || !node1)
		return false; // error occurred!!

	// check transition length
	{
		//int idx00, lv00, xloc00, yloc00;
		//int idx01, lv01, xloc01, yloc01;
		//PARSE_QGID(id0, idx00, lv00, xloc00, yloc00);
		//PARSE_QGID(id1, idx01, lv01, xloc01, yloc01);

		int idx0, idx1;
		PARSE_QGID_INDEX(id0, idx0);
		PARSE_QGID_INDEX(id1, idx1);
		const float len = node0->data.table[idx0].pos.Distance(
			node1->data.table[idx1].pos
		);
		if (len > 100.0f)
			return true; // error, too long
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
	sTransition *ptr0 = FindTransition(vtx0, id1);
	if (!ptr0)
		AddVertexTransition(vtx0, tr0);
		//return false; // already exist transition

	sTransition tr1;
	tr1.to = id0;
	if (!vtx1->trs)
	{
		vtx1->trCnt = 0;
		vtx1->trCapa = sVertex::DEFAULT_TRANSITION;
		vtx1->trs = new sTransition[sVertex::DEFAULT_TRANSITION];
		ZeroMemory(vtx1->trs, sizeof(sTransition) * sVertex::DEFAULT_TRANSITION);
	}
	sTransition *ptr1 = FindTransition(vtx1, id0);
	if (!ptr1)
		AddVertexTransition(vtx1, tr1);

	if (node0->data.table.size() != node0->data.vertices.size())
	{
		int a = 0;
	}
	if (node1->data.table.size() != node1->data.vertices.size())
	{
		int a = 0;
	}


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
	const auto result = cQuadTree<>::GetNodeLevel(rect);
	int level = std::get<0>(result);
	int xLoc = std::get<1>(result);
	int yLoc = std::get<2>(result);
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
// return = true : smooth and moving point to another node
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

	// calc mapping ids
	m_mappingIds.clear();
	MovePoint(node0, idx0);
	map<qgid, qgid> ids0 = m_mappingIds;
	MovePoint(node1, idx1);

	// copy mappingIds
	for (auto &kv : ids0)
		m_mappingIds[kv.first] = kv.second;

	m_isDivide = !m_mappingIds.empty();
	return !m_mappingIds.empty();
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

	//for (auto &vtx : node->data.vertices)
	for (uint k=0; k < node->data.vertices.size(); ++k)
	{
		auto &vtx = node->data.vertices[k];

		int idx0, lv0, x0, y0;
		PARSE_QGID(vtx.id, idx0, lv0, x0, y0);
		const Vector3 &p0 = node->data.table[idx0].pos;

		for (uint i = 0; i < vtx.trCnt; ++i)
		{
			Vector3 p1;
			const sTransition &tr = vtx.trs[i];
			if (COMPARE_QID(vtx.id, tr.to))
			{
				int idx11, lv1, x1, y1;
				PARSE_QGID(tr.to, idx11, lv1, x1, y1);

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
