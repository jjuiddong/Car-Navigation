//
// 2020-10-23, jjuiddong
// Optimize History file read/write
//	- ASCII format
//	- process path file listing
//	- complete process? or line number
//
// example
//	- filename1, complete
//	- filename2, complete
//	- filename3, processing, 10
//
#pragma once


namespace optimize
{
	class cOptimizeHistoryFile
	{
	public:
		struct sRow {
			string fileName;
			bool isComplete;
			int line;
		};

		cOptimizeHistoryFile(const StrPath &fileName = "");
		virtual ~cOptimizeHistoryFile();

		bool Read(const StrPath &fileName);
		bool Write(const StrPath &fileName);
		bool IsLoad();
		void Clear();


	public:
		vector<sRow> m_table;
	};
}

