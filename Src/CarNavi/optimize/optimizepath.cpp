
#include "stdafx.h"
#include "optimizepath.h"
#include "historyfile.h"

using namespace optimize;


cOptimizePath::cOptimizePath()
	: m_state(State::Wait)
	, m_curCalcCount(0)
	, m_totalCalcCount(0)
	, m_calcRowCount(0)
	, m_tableCount(0)
	, m_progress(0)
	, m_renderer(nullptr)
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

	// convert path file -> *.3dpath
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
	}
	//

	m_renderer = &renderer;
	m_state = State::Run;
	m_thread = std::thread(cOptimizePath::ThreadProc, this);
	return true;
}


// to debugging
// render QuadTree Graph, Quad
bool cOptimizePath::RenderQTreeGraph(graphic::cRenderer &renderer
	, cTerrainQuadTree &terrain
	, const bool showQuad, const bool showGraph)
{
	if (m_state != State::Finish)
		return false; // not finish work

	if (showQuad)
		RenderQuad(renderer, terrain);
	if (showGraph)
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

	cFrustum frustum;
	frustum.SetFrustum(GetMainCamera().GetViewProjectionMatrix());

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

		const float maxH = node ? terrain.m_tileMgr->GetMaximumHeight(node->level
			, node->xLoc, node->yLoc) : cHeightmap2::DEFAULT_H;
		const sRectf rect = qtree->GetNodeRect(node);
		if (!terrain.IsContain(frustum, rect, maxH))
			continue;

		// leaf node?
		if (!node->children[0] || (node->level <= ROOT_LINE_LEVEL))
		{
			if (!node->data.lineList)
				continue; // no lines

			if (!node->data.lineList->m_lines.empty())
			{
				node->data.lineList->UpdateBuffer(renderer);
				node->data.lineList->ClearLines();
			}
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

	// get total calc count
	opt->m_totalCalcCount = 0;
	for (auto &fileName : files)
		if (!history.IsComplete(fileName))
			opt->m_totalCalcCount++;
	//

	cPathList pathList(history, files);

	const float NEAR_LEN = 0.5f;
	while (!pathList.IsEnd())
	{
		if (opt->m_state == State::Stop)
			break; // finish thread?

		qgraph.MergePath(pathList, NEAR_LEN);
		opt->m_progress = pathList.m_progress;
	}

	qgraph.WriteFile();
	history.Write("optimize_history.txt");

	qgraph.ReadFile();
	qgraph.CreateGraphLineAll(*opt->m_renderer, ROOT_LINE_LEVEL);

	if (opt->m_state != State::Stop)
		opt->m_state = State::Finish;

	return 1;
}

