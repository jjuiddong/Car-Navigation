
#include "stdafx.h"
#include "qterrain.h"

using namespace gis;
using namespace graphic;


cQTerrain::cQTerrain()
	: m_showQuadTree(false)
	, m_showWireframe(false)
	, m_isShowLevel(false)
	, m_isShowLoc(false)
	, m_nodeBuffer(nullptr)
	, m_stack(nullptr)
{
}

cQTerrain::~cQTerrain()
{
}


// create quadtree terrain
bool cQTerrain::Create(graphic::cRenderer &renderer)
{
	sVertexNormTex vtx;
	vtx.p = Vector3(0, 0, 0);
	vtx.n = Vector3(0, 1, 0);
	vtx.u = 0;
	vtx.v = 0;
	m_vtxBuff.Create(renderer, 1, sizeof(sVertexNormTex), &vtx);

	const StrPath fileName = cResourceManager::Get()->GetResourceFilePath(
		"shader11/tess-pos-norm-tex.fxo");
	if (!m_shader.Create(renderer, fileName, "Unlit"
		, eVertexType::POSITION | eVertexType::NORMAL | eVertexType::TEXTURE0))
	{
		return false;
	}

	m_mtrl.InitWhite();
	m_mtrl.m_specular = Vector4(0, 0, 0, 1);

	// root node 10 x 5 tiles
	sRectf rootRect = sRectf::Rect(0, 0, 1 << MAX_QTREE_LEVEL, 1 << MAX_QTREE_LEVEL);
	m_qtree.m_rect = sRectf::Rect(0, 0, rootRect.right * 10, rootRect.bottom * 5);

	m_text.Create(renderer, 18.f, true, "Consolas");

	return true;
}


// update quadtree terrain
void cQTerrain::Update(graphic::cRenderer &renderer,
	const Vector2d &camLonLat, const float deltaSeconds)
{


}


// render quadtree terrain
void cQTerrain::Render(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const graphic::cFrustum &frustum
	, const Ray &ray
	, const Ray &mouseRay)
{


}


// build quad tree
void cQTerrain::BuildQuadTree(const graphic::cFrustum &frustum, const Ray &ray)
{
	m_qtree.Clear(false);
	if (!m_nodeBuffer)
		m_nodeBuffer = new sQNode<sTileData>[1024 * 2];

	int nodeCnt = 0;
	sQNode<sTileData> *node = &m_nodeBuffer[nodeCnt++];
	node->xLoc = 0;
	node->yLoc = 0;
	node->level = 0;
	m_qtree.Insert(node);

	if (!m_stack)
		m_stack = new sStackData[MAX_STACK];

	int sp = 0;
	sRectf rect = m_qtree.GetNodeRect(node);
	m_stack[sp++] = { rect, node->level, node };

	while (sp > 0)
	{
		const sRectf rect = m_stack[sp - 1].rect;
		const int lv = m_stack[sp - 1].level;
		sQNode<sTileData> *parentNode = m_stack[sp - 1].node;
		--sp;

		//const float maxH = parentNode ? m_tileMgr->GetMaximumHeight(parentNode->level
		//	, parentNode->xLoc, parentNode->yLoc) : cHeightmap2::DEFAULT_H;
		const float maxH = cHeightmap2::DEFAULT_H;

		//if (!IsContain(frustum, rect, maxH))
		//	continue;

		//const float hw = rect.Width() / 2.f;
		//const float hh = rect.Height() / 2.f;
		//const bool isMinSize = hw < 1.f;
		//if (isMinSize)
		//	continue;

		//bool isSkipThisNode = false;
		//{
		//	const Vector3 center = Vector3(rect.Center().x, maxH, rect.Center().y);
		//	const float distance = center.Distance(ray.orig);
		//	const int curLevel = GetLevel(distance);
		//	isSkipThisNode = (lv >= curLevel);
		//}

		//Vector3 childPoint[4]; // Rect의 꼭지점 4개
		//childPoint[0] = Vector3(rect.left, 0, rect.top);
		//childPoint[1] = Vector3(rect.right, 0, rect.top);
		//childPoint[2] = Vector3(rect.left, 0, rect.bottom);
		//childPoint[3] = Vector3(rect.right, 0, rect.bottom);

		//bool isChildShow = false;
		//if (isSkipThisNode && (hw > 2.f)) // test child node distance
		//{
		//	for (int i = 0; i < 4; ++i)
		//	{
		//		const float distance = childPoint[i].Distance(ray.orig);
		//		const int curLevel = GetLevel(distance);
		//		if (lv < curLevel)
		//		{
		//			isChildShow = true;
		//			break;
		//		}
		//	}
		//}

		//if (isSkipThisNode && !isChildShow)
		//	continue;

		//for (int i = nodeCnt; i < nodeCnt + 4; ++i)
		//{
		//	m_nodeBuffer[i].parent = NULL;
		//	m_nodeBuffer[i].children[0] = NULL;
		//	m_nodeBuffer[i].children[1] = NULL;
		//	m_nodeBuffer[i].children[2] = NULL;
		//	m_nodeBuffer[i].children[3] = NULL;
		//	m_nodeBuffer[i].data.tile = NULL;
		//}

		//sQuadTreeNode<sQuadData> *pp[4] = { &m_nodeBuffer[nodeCnt]
		//	, &m_nodeBuffer[nodeCnt + 1]
		//	, &m_nodeBuffer[nodeCnt + 2]
		//	, &m_nodeBuffer[nodeCnt + 3] };
		//m_qtree.InsertChildren(parentNode, pp);
		//nodeCnt += 4;

		////m_qtree.InsertChildren(parentNode);

		//g_stack[sp++] = { sRectf::Rect(rect.left, rect.top, hw, hh), lv + 1, parentNode->children[0] };
		//g_stack[sp++] = { sRectf::Rect(rect.left + hw, rect.top, hw, hh), lv + 1, parentNode->children[1] };
		//g_stack[sp++] = { sRectf::Rect(rect.left, rect.top + hh, hw, hh), lv + 1, parentNode->children[2] };
		//g_stack[sp++] = { sRectf::Rect(rect.left + hw, rect.top + hh, hw, hh), lv + 1, parentNode->children[3] };

		//assert(sp < MAX_STACK);
	}

}


// frustum 안에 rect가 포함되면 true를 리턴한다.
inline bool cQTerrain::IsContain(const graphic::cFrustum &frustum, const sRectf &rect
	, const float maxH)
{
	const float hw = rect.Width() / 2.f;
	const float hh = rect.Height() / 2.f;
	const Vector3 center = Vector3(rect.Center().x, 0, rect.Center().y);
	cBoundingBox bbox;
	bbox.SetBoundingBox(center + Vector3(0, maxH / 2.f, 0)
		, Vector3(hw, maxH / 2.f, hh)
		, Quaternion());
	const bool reval = frustum.FrustumAABBIntersect(bbox);
	return reval;
}


void cQTerrain::Clear()
{
	SAFE_DELETEA(m_nodeBuffer);
	SAFE_DELETEA(m_stack);
}
