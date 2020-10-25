
#include "stdafx.h"
#include "optimizepath.h"
#include "historyfile.h"

using namespace optimize;


cOptimizePath::cOptimizePath()
	: m_isLoop(true)
	, m_pointMapper(nullptr)
{
	m_stack = new sStackData[256];
}

cOptimizePath::~cOptimizePath()
{
	Clear();
}


// optimize path
bool cOptimizePath::Optimize(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain)
{
	if (!m_pointMapper)
		m_pointMapper = new cPointMapper();
	if (!m_qtreeGraph)
		m_qtreeGraph = new cQTreeGraph();

	Cancel();

	// check to process optimize path file
	cOptimizeHistoryFile history("optimize_history.txt");

	list<string> files;
	list<string> exts;
	exts.push_back(".txt");
	common::CollectFiles(exts, "path", files);

	for (auto &fileName : files)
	{
		// if exist *.3dpath file, read this file. (binary format)
		const StrPath binPathFileName = StrPath(fileName).GetFileNameExceptExt2() 
			+ ".3dpath";
		if (!binPathFileName.IsFileExist())
		{
			// if not exist *.3dpath file, generate file
			cPathFile pathFile(fileName);
			if (!pathFile.IsLoad())
				continue; // error occurred!!
			pathFile.Write3DPathFile(renderer, terrain, binPathFileName);
		}

		if (!binPathFileName.IsFileExist())
			continue; // error occurred!!

		// read binary format path file (*.3dpath)
		cBinPathFile pathFile(binPathFileName);
		if (!pathFile.IsLoad())
			continue; // error occurred!!

		qgid id0 = 0;
		for (auto &row : pathFile.m_table)
		{
			sEdge edge = m_qtreeGraph->FindNearEdge(row.pos, 0.5f);
			if ((edge.from != 0) && (edge.to != 0))
			{
				m_qtreeGraph->SmoothEdge(row.pos, edge);
				if (id0 > 0)
					m_qtreeGraph->AddTransition(id0, edge.to);
				id0 = edge.to;
				continue;
			}

			const qgid id1 = m_qtreeGraph->AddPoint(row.pos);
			if (m_qtreeGraph->m_isDivide)
			{
				auto it = m_qtreeGraph->m_mappingIds.find(id0);
				if (it != m_qtreeGraph->m_mappingIds.end())
					id0 = it->second;
			}

			if (id0 != 0)
			{
				m_qtreeGraph->AddTransition(id0, id1);
			}

			id0 = id1;
		}
	}

	//m_isLoop = true;
	//m_thread = std::thread(cOptimizePath::ThreadProc, this);
	return true;
}


// to debugging
// render QuadTree Graph, Quad
bool cOptimizePath::RenderQTreeGraph(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain)
{
	RenderQuad(renderer, terrain);
	RenderGraph(renderer, terrain);
	return true;
}


// render quadtree quad
bool cOptimizePath::RenderQuad(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain)
{
	using namespace graphic;

	cShader11 *shader = renderer.m_shaderMgr.FindShader(eVertexType::POSITION);
	assert(shader);
	shader->SetTechnique("Light");
	shader->Begin();
	shader->BeginPass(renderer, 0);

	struct sInfo
	{
		sRectf r;
		sQuadTreeNode<sNode> *node;
	};
	sInfo showRects[512];
	int showRectCnt = 0;
	vector<std::pair<sRectf, cColor>> ars;

	int sp = 0;
	for (auto &kv : m_qtreeGraph->m_qtrees)
	{
		cQuadTree<sNode> *qtree = kv.second;
		for (auto &node : qtree->m_roots)
		{
			sRectf rect = qtree->GetNodeRect(node);
			m_stack[sp++] = { rect, node->level, qtree, node };
		}
	}

	// Render Quad-Tree
	while (sp > 0)
	{
		cQuadTree<sNode> *qtree = m_stack[sp - 1].qtree;
		sQuadTreeNode<sNode> *node = m_stack[sp - 1].node;
		--sp;

		//const float maxH = node ? terrain.m_tileMgr->GetMaximumHeight(node->level
		//	, node->xLoc, node->yLoc) : cHeightmap2::DEFAULT_H;
		const sRectf rect = qtree->GetNodeRect(node);
		//const bool isShow = IsContain(frustum, rect, maxH);
		//if (!isShow)
		//	continue;

		// leaf node?
		if (!node->children[0])
		{
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
			{
				if (node->children[i])
				{
					m_stack[sp].qtree = qtree;
					m_stack[sp].node = node->children[i];
					++sp;
				}
			}
		}
	}

	// Render Show QuadNode
	{
		CommonStates state(renderer.GetDevice());
		renderer.GetDevContext()->RSSetState(state.CullNone());
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);
		for (int i = 0; i < showRectCnt; ++i)
		{
			const sRectf &rect = showRects[i].r;
			RenderRect3D(renderer, 0.f, rect, cColor::WHITE);
		}
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
		renderer.GetDevContext()->RSSetState(state.CullCounterClockwise());
	}

	return true;
}


// render quadtree - graph
bool cOptimizePath::RenderGraph(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain)
{
	using namespace graphic;

	cShader11 *shader = renderer.m_shaderMgr.FindShader(eVertexType::POSITION);
	assert(shader);
	shader->SetTechnique("Light");
	shader->Begin();
	shader->BeginPass(renderer, 0);

	struct sInfo
	{
		sRectf r;
		sQuadTreeNode<sNode> *node;
	};
	sInfo showRects[512];
	int showRectCnt = 0;
	vector<std::pair<sRectf, cColor>> ars;

	int sp = 0;
	for (auto &kv : m_qtreeGraph->m_qtrees)
	{
		cQuadTree<sNode> *qtree = kv.second;
		for (auto &node : qtree->m_roots)
		{
			sRectf rect = qtree->GetNodeRect(node);
			m_stack[sp++] = { rect, node->level, qtree, node };
		}
	}

	while (sp > 0)
	{
		cQuadTree<sNode> *qtree = m_stack[sp - 1].qtree;
		sQuadTreeNode<sNode> *node = m_stack[sp - 1].node;
		--sp;

		//const float maxH = node ? terrain.m_tileMgr->GetMaximumHeight(node->level
		//	, node->xLoc, node->yLoc) : cHeightmap2::DEFAULT_H;
		const sRectf rect = qtree->GetNodeRect(node);
		//const bool isShow = IsContain(frustum, rect, maxH);
		//if (!isShow)
		//	continue;

		// leaf node?
		if (!node->children[0])
		{
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
			{
				if (node->children[i])
				{
					m_stack[sp].qtree = qtree;
					m_stack[sp].node = node->children[i];
					++sp;
				}
			}
		}
	}

	// Render Graph
	{
		CommonStates state(renderer.GetDevice());
		renderer.GetDevContext()->RSSetState(state.CullNone());
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);
		for (int i = 0; i < showRectCnt; ++i)
		{
			sQuadTreeNode<sNode> *node = showRects[i].node;
			if (node->data.vertices.empty())
				continue;
			if (!node->data.lineList)
				m_qtreeGraph->CreateGraphLines(renderer, node);
			node->data.lineList->Render(renderer);
		}
		renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
		renderer.GetDevContext()->RSSetState(state.CullCounterClockwise());
	}

	return true;
}


// render quadtree quad to debugging
void cOptimizePath::RenderRect3D(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const sRectf &rect
	, const graphic::cColor &color
)
{
	using namespace graphic;

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


// cancel job
bool cOptimizePath::Cancel()
{
	m_isLoop = false;
	if (m_thread.joinable())
		m_thread.join();
	return true;
}


// read optimize path
bool cOptimizePath::ReadOptimizePath(const StrPath &fileName)
{

	return true;
}


// write optimize path
// generate optimize path from quadtree graph
bool cOptimizePath::WriteOptimizePath(const StrPath &fileName)
{



	return true;
}


// clear optimize path class
void cOptimizePath::Clear()
{
	SAFE_DELETE(m_qtreeGraph);
	SAFE_DELETE(m_pointMapper);
	SAFE_DELETEA(m_stack);
}


// thread procedure
int cOptimizePath::ThreadProc(cOptimizePath *optimizePath)
{
	while (optimizePath->m_isLoop)
	{


		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10ms);
	}

	return 1;
}

