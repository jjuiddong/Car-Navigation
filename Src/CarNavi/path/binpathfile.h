//
// 2020-10-23, jjuiddong
// binary path file read/write
//	- *.3dpath
//	- binary format
//		- | 4 byte (data size) |
//		- | 8 byte (date time) | 12 byte (3d position) |
//		- ...
#pragma once


class cBinPathFile
{
public:
	struct sRow {
		uint64 dateTime;
		Vector3 pos;
		float dummy; // 8byte alignment
	};

	cBinPathFile(const StrPath &fileName = "");
	virtual ~cBinPathFile();

	bool Read(const StrPath &fileName);
	bool IsLoad() const;
	void Clear();


public:
	vector<sRow> m_table;
};
