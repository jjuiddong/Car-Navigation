//
// 2019-08-10, jjuiddong
// land mark file read/write
//
// file format
// land mark name1, longitude, latitude
// land mark name2, longitude, latitude
// land mark name3, longitude, latitude
//
#pragma once


class cLandMark
{
public:
	struct sLandMark {
		StrId name;
		Vector2d lonLat;
	};

	cLandMark();
	virtual ~cLandMark();

	bool Read(const StrPath &fileName);
	bool Write(const StrPath &fileName);
	bool AddLandMark(const StrId &name, const Vector2d &lonLat);
	void Clear();


public:
	vector<sLandMark> m_landMarks;
};
