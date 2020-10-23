
#include "stdafx.h"
#include "pointmapper.h"

using namespace optimize;

cPointMapper::cPointMapper()
{
}

cPointMapper::~cPointMapper()
{
	Clear();
}


// read point mapper data
bool cPointMapper::Read(const StrPath &fileName)
{

	return true;
}


// write point mapper data
bool cPointMapper::Write(const StrPath &fileName)
{

	return true;
}


// add point
// pos: relation pos
Vector3 cPointMapper::addPoint(const Vector3 &pos)
{
	const Vector3 gpos = cQuadTree<>::GetGlobalPos(pos);
	const sRectf rect = sRectf::Rect(gpos.x, gpos.z, 0, 0);
	int level = TREE_LEVEL;
	const auto result = cQuadTree<>::GetNodeLocation(rect, level);
	int xLoc = std::get<0>(result);
	int yLoc = std::get<1>(result);
	if (xLoc < 0)
		return Vector3::Zeroes; // error occurred!!

	cQuadTree<sMapping> *qtree = findQuadTree(level, xLoc, yLoc);
	if (!qtree)
		return Vector3::Zeroes; // error occurred!!

	// empty tree? create root node
	if (qtree->m_roots.empty())
	{
		sQuadTreeNode<sMapping> *root = new sQuadTreeNode<sMapping>();
		root->level = level;
		root->xLoc = xLoc;
		root->yLoc = yLoc;
		qtree->m_rootRect = cQuadTree<sMapping>::GetNodeRect(level, xLoc, yLoc);
		qtree->m_roots.push_back(root);
	}

	return addPointInBestNode(qtree, rect, pos);
}


// find quadtree from lv, xloc, yloc
cQuadTree<sMapping>* cPointMapper::findQuadTree(int lv, int xLoc, int yLoc)
{
	const int64 key = cQuadTree<>::MakeKey(lv, xLoc, yLoc);
	auto it = m_qtrees.find(key);
	if (m_qtrees.end() != it)
		return it->second;

	cQuadTree<sMapping> *newQtree = new cQuadTree<sMapping>();
	m_qtrees[key] = newQtree;
   	return newQtree;
}


// add point in best quadtree node
// pos: relation pos
Vector3 cPointMapper::addPointInBestNode(cQuadTree<sMapping> *qtree
	, const sRectf &rect, const Vector3 &pos)
{
	sQuadTreeNode<sMapping> *rootNode = qtree->m_roots[0];
	int level = rootNode->level;
	int xLoc = rootNode->xLoc;
	int yLoc = rootNode->yLoc;
	Vector3 retVal = pos;

	// find best tree node
	sQuadTreeNode<sMapping> *node = qtree->m_roots[0];
	while (1)
	{
		const bool isRoot = (level == TREE_LEVEL);
		const bool isOverflow = isRoot ? (node->data.table.size() > MAX_TABLESIZE)
			: (node->data.indices.size() > MAX_TABLESIZE);

		if (isOverflow && (level < (cQuadTree<>::MAX_LEVEL - 1)))
		{
			++level;
			const auto res = cQuadTree<>::GetNodeLocation(rect, level);
			int x = std::get<0>(res);
			int y = std::get<1>(res);
			sQuadTreeNode<sMapping> *child = qtree->GetNode(level, x, y);
			if (child)
			{
				node = child;
				if (child->data.indices.size() > MAX_TABLESIZE)
					continue; // goto children
			}
			else
			{
				// make child node and table
				qtree->InsertChildren(node);

				auto fnClone = [&](const int idx) {
					const Vector3 p = rootNode->data.table[idx].pos;
					const Vector3 gp = cQuadTree<>::GetGlobalPos(p);
					const sRectf r = sRectf::Rect(gp.x, gp.z, 0, 0);
					const auto rs = cQuadTree<>::GetNodeLocation(r, level);
					const int x0 = std::get<0>(rs);
					const int y0 = std::get<1>(rs);
					sQuadTreeNode<sMapping> *pn = qtree->GetNode(level, x0, y0);
					if (!pn)
						return; // error occurred!!
					pn->data.indices.push_back(idx);
				};

				if (isRoot)
				{
					for (uint i = 0; i < node->data.table.size(); ++i)
						fnClone((int)i);
				}
				else
				{
					for (auto idx : node->data.indices)
						fnClone(idx);
				}

				node = qtree->GetNode(level, x, y);
				continue; // goto children
			}
		}
		if (!node)
			break; // error occurred!!

		// find duplicate pos
		int findIdx = -1;
		for (auto idx : node->data.indices)
		{
			sMappingPos &mpos = rootNode->data.table[idx];
			const Vector3 p = mpos.pos;
			const float dist = pos.Distance(p);
			
			// same position?
			if (dist < 0.1f) 
			{
				// average position, update
				mpos.pos = (p / (float)(mpos.cnt + 1))
					+ (pos / (float)(mpos.cnt + 1));
				mpos.cnt += 1;
				retVal = mpos.pos;
				findIdx = idx;
				break;
			}
		}

		// no near position, add new pos
		if (findIdx < 0)
		{
			rootNode->data.table.push_back({ 1, pos });
			node->data.indices.push_back(rootNode->data.table.size() - 1);
		}

		break; // complete
	}

	return retVal;
}


// clear all 
void cPointMapper::Clear()
{
	for (auto &kv : m_qtrees)
		delete kv.second;
	m_qtrees.clear();
}
