
#include "stdafx.h"
#include "optimizepath.h"
#include "historyfile.h"

using namespace optimize;


cOptimizePath::cOptimizePath()
	: m_state(State::Wait)
	, m_renderer(nullptr)
	, m_terrain(nullptr)
{
	m_stack = new sStackData[512];
}

cOptimizePath::~cOptimizePath()
{
	Clear();
}


// optimize path
bool cOptimizePath::Optimize(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain)
{
	if (!m_qtreeGraph)
		m_qtreeGraph = new cQTreeGraph();

	Cancel();

	m_renderer = &renderer;
	m_terrain = &terrain;

	m_state = State::Run;
	m_thread = std::thread(cOptimizePath::ThreadProc, this);
	m_thread.join();

	return true;
}


// to debugging
// render QuadTree Graph, Quad
bool cOptimizePath::RenderQTreeGraph(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain)
{
	if (m_state != State::Finish)
		return false; // not finish work

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

	CommonStates state(renderer.GetDevice());
	renderer.GetDevContext()->RSSetState(state.CullNone());
	renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);

	int sp = 0;
	for (auto &kv : m_qtreeGraph->m_roots)
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
			RenderRect3D(renderer, 0.f, rect, cColor::WHITE);
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

	renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
	renderer.GetDevContext()->RSSetState(state.CullCounterClockwise());
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

	CommonStates state(renderer.GetDevice());
	renderer.GetDevContext()->RSSetState(state.CullNone());
	renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);

	int sp = 0;
	for (auto &kv : m_qtreeGraph->m_roots)
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
			if (node->data.vertices.empty())
				continue;

			if (!node->data.lineList)
				m_qtreeGraph->CreateGraphLines(renderer, node);
			node->data.lineList->Render(renderer);
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

	renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
	renderer.GetDevContext()->RSSetState(state.CullCounterClockwise());
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
	m_state = State::Stop;
	if (m_thread.joinable())
		m_thread.join();
	return true;
}


// clear optimize path class
void cOptimizePath::Clear()
{
	Cancel();
	SAFE_DELETE(m_qtreeGraph);
	SAFE_DELETEA(m_stack);
}


// thread procedure
int cOptimizePath::ThreadProc(cOptimizePath *optimizePath)
{
	cOptimizePath *opt = optimizePath;
	cOptimizeHistoryFile &history = optimizePath->m_history;
	cQTreeGraph &qgraph = *optimizePath->m_qtreeGraph;

	// check to process optimize path file
	history.Read("optimize_history.txt");

	list<string> files;
	list<string> exts;
	exts.push_back(".txt");
	common::CollectFiles(exts, "path", files);

	for (auto &fileName : files)
	{
		if (opt->m_state == State::Stop)
			break; // finish thread?

		if (history.IsComplete(fileName))
			continue;

		// if exist *.3dpath file, read this file. (binary format)
		const StrPath binPathFileName = StrPath(fileName).GetFileNameExceptExt2()
			+ ".3dpath";
		if (!binPathFileName.IsFileExist())
		{
			// if not exist *.3dpath file, generate file
			cPathFile pathFile(fileName);
			if (!pathFile.IsLoad())
				continue; // error occurred!!
			pathFile.Write3DPathFile(*opt->m_renderer, *opt->m_terrain, binPathFileName);
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
			if (opt->m_state == State::Stop)
				break; // finish thread?

			sEdge edge = qgraph.FindNearEdge(row.pos, 0.5f);
			if ((edge.from != 0) && (edge.to != 0))
			{
				if (qgraph.SmoothEdge(row.pos, edge))
				{
					// recalc near edge
					edge = qgraph.FindNearEdge(row.pos, 0.5f);

					if (qgraph.m_isDivide)
					{
						auto it = qgraph.m_mappingIds.find(id0);
						if (it != qgraph.m_mappingIds.end())
							id0 = it->second;
					}
				}

				if ((edge.from != 0) && (edge.to != 0))
				{
					if (id0 > 0)
					{
						if (qgraph.AddTransition(id0, edge.to))
							id0 = edge.to;
						else
							id0 = 0;
					}
				}
				continue;
			}

			const qgid id1 = qgraph.AddPoint(row.pos);
			if (qgraph.m_isDivide)
			{
				auto it = qgraph.m_mappingIds.find(id0);
				if (it != qgraph.m_mappingIds.end())
					id0 = it->second;
			}

			if (id0 != 0)
			{
				qgraph.AddTransition(id0, id1);
			}
			id0 = id1;
		}

		history.AddHistory(fileName, true, 0);
	}

	qgraph.WriteFile();
	history.Write("optimize_history.txt");

	qgraph.ReadFile();

	if (opt->m_state != State::Stop)
		opt->m_state = State::Finish;

	return 1;
}

