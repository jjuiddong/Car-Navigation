//
// 2018-06-18, jjuiddong
// flight route
// https://www.geoplaner.com/ ���� ������ ��� xml ������ �о �����Ѵ�.
//	- ���浵 �迭
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
