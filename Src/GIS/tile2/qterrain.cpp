
#include "stdafx.h"
#include "qterrain.h"

using namespace graphic;
using namespace gis2;


cQTerrain::cQTerrain()
	: m_showQuadTree(false)
	, m_showWireframe(false)
	, m_isShowLevel(true)
	, m_isShowLoc(true)
	, m_nodeBuffer(nullptr)
	, m_stack(nullptr)
	, m_distanceLevelOffset(20)
{
}

cQTerrain::~cQTerrain()
{
	Clear();
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
	m_tileMgr.Update(renderer, *this, camLonLat, deltaSeconds);
}


// render quadtree terrain
void cQTerrain::Render(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const graphic::cFrustum &frustum
	, const Ray &ray
	, const Ray &mouseRay)
{
	BuildQuadTree(frustum, ray);

	if (!m_qtree.m_root)
		return; // error

	RenderTessellation(renderer, deltaSeconds, frustum);
	RenderQuad(renderer, deltaSeconds, frustum, mouseRay);
}


// render seamless terrain
void cQTerrain::RenderTessellation(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const graphic::cFrustum &frustum)
{
	m_shader.SetTechnique("ArcGis");
	m_shader.Begin();
	m_shader.BeginPass(renderer, 0);

	renderer.m_cbLight.Update(renderer, 1);
	renderer.m_cbMaterial = m_mtrl.GetMaterial();
	renderer.m_cbMaterial.Update(renderer, 2);

	int sp = 0;
	sRectf rect = m_qtree.GetNodeRect(m_qtree.m_root);
	m_stack[sp++] = { rect, 0, m_qtree.m_root };

	while (sp > 0)
	{
		sQNode<sTileData> *node = m_stack[sp - 1].node;
		--sp;

		const sRectf rect = m_qtree.GetNodeRect(node);
		cQTile *tile = m_tileMgr.GetTile(renderer, node->level, node->x, node->y, rect);
		if (!tile)
			continue; // error
		node->data.tile = tile;
		const float maxH = tile->m_hmap? tile->m_hmap->m_maxH : 
			(tile->m_replaceHmap? tile->m_replaceHmap->m_maxH : cHeightmap3::DEFAULT_H);
		if (!IsContain(frustum, rect, maxH))
			continue;

		// leaf node?
		if (!node->children[0])
		{
			//tile->m_renderFlag = m_tileRenderFlag;
			m_tileMgr.LoadResource(renderer, *this, *tile, node->level, node->x, node->y, rect);

			//if (m_showTile)
			tile->Render(renderer, deltaSeconds);
		}
		else
		{
			//const sRectf rect = m_qtree.GetNodeRect(node);
			//cQTile *tile = m_tileMgr.GetTile(renderer, node->level, node->x, node->y, rect);
			//tile->m_renderFlag = m_tileRenderFlag;
			//node->data.tile = tile;
			m_tileMgr.LoadResource(renderer, *this, *tile, node->level, node->x, node->y, rect);

			for (int i = 0; i < 4; ++i)
				if (node->children[i])
					m_stack[sp++].node = node->children[i];
		}
	}

	renderer.UnbindShaderAll();
}


// QaudTree 정보를 출력한다.
// QuadRect, Level, xLoc, yLoc
void cQTerrain::RenderQuad(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const graphic::cFrustum &frustum
	, const Ray &ray)
{
	cShader11 *shader = renderer.m_shaderMgr.FindShader(eVertexType::POSITION);
	assert(shader);
	shader->SetTechnique("Light");// "Unlit");
	shader->Begin();
	shader->BeginPass(renderer, 0);

	struct sInfo
	{
		sRectf r;
		sQNode<sTileData> *node;
	};
	sInfo showRects[512];
	int showRectCnt = 0;
	vector<std::pair<sRectf, cColor>> ars;

	int sp = 0;
	sRectf rect = m_qtree.GetNodeRect(m_qtree.m_root);
	m_stack[sp++] = { rect, 0, m_qtree.m_root };

	// Render Quad-Tree
	while (sp > 0)
	{
		sQNode<sTileData> *node = m_stack[sp - 1].node;
		--sp;

		cQTile *tile = node->data.tile;
		if (!tile)
			continue;
		const float maxH = tile->m_hmap ? tile->m_hmap->m_maxH :
			(tile->m_replaceHmap ? tile->m_replaceHmap->m_maxH : cHeightmap3::DEFAULT_H);

		const sRectf rect = m_qtree.GetNodeRect(node);
		const bool isShow = IsContain(frustum, rect, maxH);
		if (!isShow)
			continue;

		// leaf node?
		if (!node->children[0])
		{
			cQuadTile *tile = nullptr;// m_tileMgr->GetTile(renderer, node->level, node->xLoc, node->yLoc, rect);
			//RenderRect3D(renderer, deltaSeconds, rect, cColor::BLACK);

			if (showRectCnt < ARRAYSIZE(showRects))
			{
				showRects[showRectCnt].r = rect;
				showRects[showRectCnt].node = node;
				showRectCnt++;
			}
		}
		else
		{
			for (int i = 0; i < 4; ++i)
				if (node->children[i])
					m_stack[sp++].node = node->children[i];
		}
	}


	// Render Show QuadNode
	CommonStates state(renderer.GetDevice());
	renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);
	for (int i = 0; i < showRectCnt; ++i)
	{
		const sRectf &rect = showRects[i].r;
		RenderRect3D(renderer, deltaSeconds, rect, cColor::WHITE);
	}

	// Render north, south, west, east quad node
	Plane ground(Vector3(0, 1, 0), 0);
	const Vector3 relPos = ground.Pick(ray.orig, ray.dir);
	const Vector3 globalPos = gis2::GetGlobalPos(relPos);
	if (sQNode<sTileData> *node = m_qtree.GetNode(sRectf::Rect(globalPos.x, globalPos.z, 0, 0)))
	{
		const sRectf rect = m_qtree.GetNodeRect(node);
		ars.push_back({ rect, cColor::WHITE });

		if (sQNode<sTileData> *north = m_qtree.GetNorthNeighbor(node))
		{
			const sRectf r = m_qtree.GetNodeRect(north);
			ars.push_back({ r, cColor::RED });
		}

		if (sQNode<sTileData> *east = m_qtree.GetEastNeighbor(node))
		{
			const sRectf r = m_qtree.GetNodeRect(east);
			ars.push_back({ r, cColor::GREEN });
		}

		if (sQNode<sTileData> *south = m_qtree.GetSouthNeighbor(node))
		{
			const sRectf r = m_qtree.GetNodeRect(south);
			ars.push_back({ r, cColor::BLUE });
		}

		if (sQNode<sTileData> *west = m_qtree.GetWestNeighbor(node))
		{
			const sRectf r = m_qtree.GetNodeRect(west);
			ars.push_back({ r, cColor::YELLOW });
		}
	}

	for (auto &data : ars)
	{
		const sRectf &r = data.first;

		CommonStates state(renderer.GetDevice());
		renderer.GetDevContext()->RSSetState(state.CullNone());
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);
		//RenderRect3D(renderer, deltaSeconds, r, data.second);
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
		renderer.GetDevContext()->RSSetState(state.CullCounterClockwise());
	}
	renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);

	// Render Show QuadNode
	if (m_isShowLevel || m_isShowLoc)
	{
		FW1FontWrapper::CFW1StateSaver stateSaver;
		stateSaver.saveCurrentState(renderer.GetDevContext());

		for (int i = 0; i < showRectCnt; ++i)
		{
			const sRectf &rect = showRects[i].r;
			const Vector3 offset = Vector3(rect.Width()*0.1f, 0, -rect.Height()*0.1f);
			const Vector2 pos = GetMainCamera().GetScreenPos(Vector3(rect.left, 0, rect.bottom) + offset);

			m_text.SetColor(cColor(m_txtColor));

			if (m_isShowLevel)
			{
				WStr32 strLv;
				strLv.Format(L"%d", showRects[i].node->level);
				m_text.SetText(strLv.c_str());
				m_text.Render(renderer, pos.x, pos.y, false, false);
			}

			if (m_isShowLoc)
			{
				WStr32 strLoc;
				strLoc.Format(L"xyLoc = {%d, %d}", showRects[i].node->x, showRects[i].node->y);
				m_text.SetText(strLoc.c_str());
				m_text.Render(renderer, pos.x, pos.y + 20, false, false);

				//if (showRects[i].node->data.tile &&
				//	showRects[i].node->data.tile->m_replaceHeightLv >= 0)
				//{
				//	strLoc.Format(L"rep h = %d", showRects[i].node->data.tile->m_replaceHeightLv);
				//	m_text.SetText(strLoc.c_str());
				//	m_text.Render(renderer, pos.x, pos.y + 40, false, false);
				//}
			}
		}
		stateSaver.restoreSavedState();
	}
}


void cQTerrain::RenderRect3D(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const sRectf &rect
	, const cColor &color
)
{
	Vector3 lines[] = {
		Vector3(rect.left, 0, rect.top)
		, Vector3(rect.right, 0, rect.top)
		, Vector3(rect.right, 0, rect.bottom)
		, Vector3(rect.left, 0, rect.bottom)
		, Vector3(rect.left, 0, rect.top)
	};

	renderer.m_rect3D.SetRect(renderer, lines, 5);

	renderer.m_cbPerFrame.m_v->mWorld = XMIdentity;
	renderer.m_cbPerFrame.Update(renderer);
	const Vector4 c = color.GetColor();
	renderer.m_cbMaterial.m_v->diffuse = XMVectorSet(c.x, c.y, c.z, c.w);
	renderer.m_cbMaterial.Update(renderer, 2);
	renderer.m_rect3D.m_vtxBuff.Bind(renderer);
	renderer.GetDevContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	renderer.GetDevContext()->Draw(renderer.m_rect3D.m_lineCount, 0);
}


// build quad tree
void cQTerrain::BuildQuadTree(const graphic::cFrustum &frustum, const Ray &ray)
{
	m_qtree.Clear(false);
	if (!m_nodeBuffer)
		m_nodeBuffer = new sQNode<sTileData>[1024 * 2];

	int nodeCnt = 0;
	sQNode<sTileData> *node = &m_nodeBuffer[nodeCnt++];
	node->x = 0;
	node->y = 0;
	node->level = 0;
	for (int i = 0; i < 4; ++i)
		node->children[i] = nullptr;
	m_qtree.Insert(node);

	if (!m_stack) {
		m_stack = new sStackData[MAX_STACK];
		ZeroMemory(m_stack, sizeof(sStackData) * MAX_STACK);
	}

	int sp = 0;
	sRectf rect = m_qtree.GetNodeRect(node);
	m_stack[sp++] = { rect, node->level, node };

	while (sp > 0)
	{
		const sRectf rect = m_stack[sp - 1].rect;
		const int lv = m_stack[sp - 1].level;
		sQNode<sTileData> *node = m_stack[sp - 1].node;
		--sp;

		cQTile *tile = node ? 
			m_tileMgr.FindTile(node->level, node->x, node->y) : nullptr;
		const float maxH = tile? 
			(tile->m_hmap ? tile->m_hmap->m_maxH :
				(tile->m_replaceHmap ? 
					tile->m_replaceHmap->m_maxH : cHeightmap3::DEFAULT_H))
			: cHeightmap2::DEFAULT_H;

		if (!IsContain(frustum, rect, maxH))
			continue;

		const float hw = rect.Width() / 2.f;
		const float hh = rect.Height() / 2.f;
		const bool isMinSize = hw < 1.f;
		if (isMinSize)
			continue;

		bool isSkipThisNode = false;
		{
			const Vector3 center = Vector3(rect.Center().x, maxH, rect.Center().y);
			const float distance = center.Distance(ray.orig);
			const int curLevel = GetLevel(distance);
			isSkipThisNode = (lv >= curLevel);
		}

		const Vector3 p0 = Vector3(rect.left, 0, rect.top);
		const Vector3 p1 = Vector3(rect.right, 0, rect.top);
		const Vector3 p2 = Vector3(rect.left, 0, rect.bottom);
		const Vector3 p3 = Vector3(rect.right, 0, rect.bottom);
		const Triangle tri0(p0, p1, p2);
		const Triangle tri1(p1, p3, p2);

		bool isChildShow = false;
		if (isSkipThisNode && (hw > 2.f)) // test child node distance
		{
			const Vector3 cp0 = tri0.GetClosesPointOnTriangle(ray.orig);
			const Vector3 cp1 = tri1.GetClosesPointOnTriangle(ray.orig);
			const float dist0 = ray.orig.Distance(cp0);
			const float dist1 = ray.orig.Distance(cp1);
			const int curLevel0 = GetLevel(dist0);
			const int curLevel1 = GetLevel(dist1);
			if ((lv < curLevel0) || (lv < curLevel1))
			{
				isChildShow = true;
			}
		}

		if (isSkipThisNode && !isChildShow)
			continue;

		for (int i = nodeCnt; i < nodeCnt + 4; ++i)
		{
			m_nodeBuffer[i].parent = NULL;
			m_nodeBuffer[i].children[0] = nullptr;
			m_nodeBuffer[i].children[1] = nullptr;
			m_nodeBuffer[i].children[2] = nullptr;
			m_nodeBuffer[i].children[3] = nullptr;
			m_nodeBuffer[i].data.tile = nullptr;
		}

		sQNode<sTileData> *pp[4] = { &m_nodeBuffer[nodeCnt]
			, &m_nodeBuffer[nodeCnt + 1]
			, &m_nodeBuffer[nodeCnt + 2]
			, &m_nodeBuffer[nodeCnt + 3] };
		m_qtree.InsertChildren(node, pp);
		nodeCnt += 4;

		m_stack[sp++] = { sRectf::Rect(rect.left, rect.top, hw, hh), lv + 1, node->children[0] };
		m_stack[sp++] = { sRectf::Rect(rect.left + hw, rect.top, hw, hh), lv + 1, node->children[1] };
		m_stack[sp++] = { sRectf::Rect(rect.left, rect.top + hh, hw, hh), lv + 1, node->children[2] };
		m_stack[sp++] = { sRectf::Rect(rect.left + hw, rect.top + hh, hw, hh), lv + 1, node->children[3] };

		assert(sp < MAX_STACK);
	}
}


// frustum 안에 rect가 포함되면 true를 리턴한다.
inline bool cQTerrain::IsContain(const graphic::cFrustum &frustum, const sRectf &rect
	, const float maxH)
{
	const float hw = rect.Width() / 2.f;
	const float hh = rect.Height() / 2.f;
	const float height = std::max(maxH, 2.f);
	const Vector3 center = Vector3(rect.Center().x, 0, rect.Center().y);
	cBoundingBox bbox;
	bbox.SetBoundingBox(center + Vector3(0, height / 2.f, 0)
		, Vector3(hw, height / 2.f, hh)
		, Quaternion());
	const bool reval = frustum.FrustumAABBIntersect(bbox);
	return reval;
}


std::pair<bool, Vector3> cQTerrain::Pick(const Ray &ray)
{
	sQNode<sTileData> *pnode = m_qtree.m_root;
	RETV(!pnode, std::make_pair(false, Vector3(0, 0, 0)));

	const Ray newRay(ray.orig - ray.dir*1000.f, ray.dir);
	sQNode<sTileData> *pickNode = nullptr;
	for (int i = 0; i < 4; ++i)
	{
		// is Leaf?
		if (!pnode->children[0])
		{
			pickNode = pnode;
			break;
		}

		auto &node = pnode->children[i];
		if (!node->data.tile)
			continue;
		auto result = node->data.tile->Pick(newRay);
		if (!result.first)
			continue;

		pnode = node;
		i = -1; // because ++i
	}

	if (!pickNode)
		pickNode = pnode;

	RETV(!pickNode, std::make_pair(false, Vector3(0, 0, 0)));

	// detail picking pickNode
	return pickNode->data.tile->Pick(newRay);
}


// return longitude, latitude correspond relation position
Vector2d cQTerrain::GetWGS84(const Vector3 &relPos)
{
	return gis2::Pos2WGS84(relPos);
}


// return 3d pos(relation coordinate) correspond longitude/latitude
Vector3 cQTerrain::Get3DPos(const Vector2d &lonLat)
{
	Vector3 relPos = gis2::WGS842Pos(lonLat);
	const Vector3 gpos = gis2::GetGlobalPos(relPos);

	// calc height
	const sQNode<sTileData> *node =
		m_qtree.GetNode(sRectf::Rect(gpos.x, gpos.z, 0, 0));
	if (node && !node->data.tile)
		node = m_qtree.GetNode(node->level - 1, node->x >> 1, node->y>> 1);
	if (node && node->data.tile)
	{
		cQTile *tile = node->data.tile;
		const float u = (relPos.x - tile->m_rect.left) / tile->m_rect.Width();
		const float v = 1.f + ((tile->m_rect.top - relPos.z) / tile->m_rect.Height());
		relPos.y = tile->GetHeight(Vector2(u, v));
	}
	return relPos;
}


// calc level by distance from camera eyepos, terrain
inline int cQTerrain::GetLevel(const float distance)
{
	int dist = (int)distance - m_distanceLevelOffset;

#define CALC(lv) \
	if ((dist >>= 1) < 1) \
	{ \
		return lv+1; \
	}

	CALC(MAX_QTREE_LEVEL);
	CALC(MAX_QTREE_LEVEL - 1);
	CALC(MAX_QTREE_LEVEL - 2);
	CALC(MAX_QTREE_LEVEL - 3);
	CALC(MAX_QTREE_LEVEL - 4);
	CALC(MAX_QTREE_LEVEL - 5);
	CALC(MAX_QTREE_LEVEL - 6);
	CALC(MAX_QTREE_LEVEL - 7);
	CALC(MAX_QTREE_LEVEL - 8);
	CALC(MAX_QTREE_LEVEL - 9);
	CALC(MAX_QTREE_LEVEL - 10);
	CALC(MAX_QTREE_LEVEL - 11);
	CALC(MAX_QTREE_LEVEL - 12);
	CALC(MAX_QTREE_LEVEL - 13);
	CALC(MAX_QTREE_LEVEL - 14);
	CALC(MAX_QTREE_LEVEL - 15);
	CALC(MAX_QTREE_LEVEL - 16);
	CALC(MAX_QTREE_LEVEL - 17);
	CALC(MAX_QTREE_LEVEL - 18);
	CALC(MAX_QTREE_LEVEL - 19);
	return 0;
}


void cQTerrain::Clear()
{
	SAFE_DELETEA(m_nodeBuffer);
	SAFE_DELETEA(m_stack);
	m_tileMgr.Clear();
	m_qtree.Clear(false);
}
