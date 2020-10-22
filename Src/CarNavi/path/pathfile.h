//
// 2019-08-10, jjuiddong
// path data file read
//
// file format
// 2019-08-04-08-20-24-056, 128.259460449218750, 37.871887207031250
// 2019-08-04-08-26-15-079, 128.259249000000011, 37.872394000000000
// 2019-08-04-08-26-27-090, 128.258723000000003, 37.872725000000003
//
#pragma once

class cPathFile
{
public:
	struct sRow {
		uint64 dateTime;
		Vector2d lonLat;
	};

	cPathFile(const StrPath &fileName = "");
	virtual ~cPathFile();

	bool Read(const StrPath &fileName);
	bool IsLoad() const;
	void Clear();


public:
	vector<sRow> m_table;
};
