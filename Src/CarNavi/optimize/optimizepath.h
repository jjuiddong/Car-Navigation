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

	class cOptimizePath
	{
	public:
		cOptimizePath();
		virtual ~cOptimizePath();

		bool Optimize(graphic::cRenderer &renderer, cTerrainQuadTree &terrain);
		bool Cancel();
		void Clear();


	protected:
		static int ThreadProc(cOptimizePath *optimizePath);


	public:
		cPointMapper *m_pointMapper;

		bool m_isLoop;
		std::thread m_thread;
	};

}
