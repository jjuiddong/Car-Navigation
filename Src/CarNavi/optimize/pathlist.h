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
	};

}
