//
// 2020-10-22, jjuiddong
// optimize path
//		- multithreading
//		- cPointMapper
//		- cOptimizePath
//
//	- path-table
//	- path-optimize
//	- optimize-sub-path
//
#pragma once


namespace optimize
{

	class cPointMapper;
	class cQTreeGraph;

	// Quad Tree Traverse Stack Memory
	struct sStackData
	{
		sRectf rect;
		int level;
		cQuadTree<sNode> *qtree;
		sQuadTreeNode<sNode> *node;
	};

	class cOptimizePath
	{
	public:
		cOptimizePath();
		virtual ~cOptimizePath();

		bool Optimize(graphic::cRenderer &renderer, cTerrainQuadTree &terrain);
		bool RenderQTreeGraph(graphic::cRenderer &renderer, cTerrainQuadTree &terrain);
		bool Cancel();
		void Clear();


	protected:
		static int ThreadProc(cOptimizePath *optimizePath);

		bool RenderQuad(graphic::cRenderer &renderer
			, cTerrainQuadTree &terrain);

		bool RenderGraph(graphic::cRenderer &renderer
			, cTerrainQuadTree &terrain);

		void RenderRect3D(graphic::cRenderer &renderer
			, const float deltaSeconds
			, const sRectf &rect
			, const graphic::cColor &color
		);


	public:
		enum class State {Wait, Run, Finish, Stop};

		State m_state;
		cOptimizeHistoryFile m_history;
		cQTreeGraph *m_qtreeGraph;
		sStackData *m_stack;
		graphic::cRenderer *m_renderer; // reference
		cTerrainQuadTree *m_terrain; // reference
		std::thread m_thread;
	};

}
