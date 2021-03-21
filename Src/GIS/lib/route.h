//
// 2018-06-18, jjuiddong
// flight route
// https://www.geoplaner.com/ 에서 생성한 경로 xml 파일을 읽어서 저장한다.
//	- 위경도 배열
//
#pragma once


class cRoute
{
public:
	cRoute();
	virtual ~cRoute();

	bool Read(const char *fileName);
	bool GeneratePath(const float GAP);
	void Clear();


public:
	vector<Vector2d> m_path; // lon, lat array
	vector<Vector2d> m_genPath; // generate route
};
