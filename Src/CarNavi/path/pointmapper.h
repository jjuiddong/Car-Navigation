//
// 2020-10-22, jjuiddong
// point mapper
//
#pragma once


// 3D position table, unique pos management
struct sMappingQuad
{
	int lv;
	int xLoc;
	int yLoc;
	bool isHasChild;
	vector<Vector3> table;
	vector<int> indices; // lv10 table pointed
};


// cPointMapper
class cPointMapper
{
public:
	cPointMapper();
	virtual ~cPointMapper();


protected:
	Vector3 addPoint(const Vector3 &pos);


public:
	map<int64, cQuadTree<sMappingQuad>*> m_qtree; //key: lv + xLoc + yLoc
};
