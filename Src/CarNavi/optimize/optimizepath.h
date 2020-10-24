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
		void RenderRect3D(graphic::cRenderer &renderer
			, const float deltaSeconds
			, const sRectf &rect
			, const graphic::cColor &color
		);


	public:
		cPointMapper *m_pointMapper;
		cQTreeGraph *m_qtreeGraph;

		bool m_isLoop;
		std::thread m_thread;
	};

}
