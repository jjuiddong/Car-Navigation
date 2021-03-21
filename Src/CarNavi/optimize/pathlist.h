//
// 2020-10-29, jjuiddong
// path list
//	- path file list
//
#pragma once


namespace optimize
{

	class cPathList
	{
	public:
		cPathList(cOptimizeHistoryFile &history, const list<string> &files);
		virtual ~cPathList();

		bool IsEnd();
		Vector3 GetNextPos();
		std::pair<Vector3, Vector3> GetNextPath();
		void Clear();


	protected:
		float CalcProgress();


	public:
		vector<string> m_files;
		cOptimizeHistoryFile &m_history;
		cBinPathFile m_pathFile;
		uint m_fileIdx;
		uint m_line;
		float m_progress; // ratio 0 ~ 1
		Vector3 m_prevPos; // previous pos
		Vector3 m_curPos; // current pos
	};

}
