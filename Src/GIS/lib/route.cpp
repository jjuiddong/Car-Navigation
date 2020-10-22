
#include "stdafx.h"
#include "route.h"

using namespace graphic;


cRoute::cRoute()
{
}

cRoute::~cRoute()
{
	Clear();
}


bool cRoute::Read(const char *fileName)
{
	Clear();

	//TiXmlDocument doc(fileName);
	//if (doc.LoadFile())
	//{
	//	TiXmlElement *tnode = doc.FirstChildElement("gpx");
	//	RETV(!tnode, false);
	//	TiXmlElement *rte = tnode->FirstChildElement("rte");
	//	RETV(!rte, false);
	//	TiXmlElement *rtept = rte->FirstChildElement("rtept");
	//	RETV(!rtept, false);

	//	while (rtept)
	//	{
	//		const char *lat = rtept->Attribute("lat");
	//		const char *lon = rtept->Attribute("lon");
	//		rtept = rtept->NextSiblingElement();
	//		m_path.push_back(Vector2d(atof(lon), atof(lat)));
	//	}
	//}

	return true;
}


// m_path에 저장된 사각영역을 기준으로 경로를 재생성한다.
// 1. 사각영역을 시계방향으로 돌면서 전체를 훑는다.
bool cRoute::GeneratePath(const float GAP)
{
	if (4 != m_path.size())
		return false;

	//
	// p0 ------- p1
	// | \     /  |
	// |  d0  d1  |
	// |     *    |
	// |   /  \   |
	// | /     \  |
	// p3 ------- p2
	//
	const Vector3 p0((float)m_path[0].x, 0, (float)m_path[0].y);
	const Vector3 p1((float)m_path[1].x, 0, (float)m_path[1].y);
	const Vector3 p2((float)m_path[2].x, 0, (float)m_path[2].y);
	const Vector3 p3((float)m_path[3].x, 0, (float)m_path[3].y);
	const Vector3 d0 = (p2 - p0).Normal();
	const Vector3 d1 = (p3 - p1).Normal();
	const Vector3 center = (p0 + p1 + p2 + p3) / 4.f;

	m_genPath.clear();
	Vector3 rect[4] = { p0, p1, p2, p3 };
	Vector3 oldRect[4];
	int loopCnt = 0;
	float gap = GAP;
	bool saveRoute = true;

	while (loopCnt < 1000)
	{
		++loopCnt; // exception infinity loop

		if (saveRoute)
		{
			for (int i = 0; i < 4; ++i)
				m_genPath.push_back(Vector2d(rect[i].x, rect[i].z));
		}

		oldRect[0] = rect[0];
		oldRect[1] = rect[1];
		oldRect[2] = rect[2];
		oldRect[3] = rect[3];
		rect[0] = rect[0] + (d0 * gap);
		rect[1] = rect[1] + (d1 * gap);
		rect[2] = rect[2] - (d0 * gap);
		rect[3] = rect[3] - (d1 * gap);

		// rect[0],[3] - rect[0],[1] 교점 찾기
		const Vector3 temp0 = oldRect[0];
		const Vector3 temp1 = oldRect[3];
		Vector3 tp0Center = (temp0 + temp1) / 2.f;
		Vector3 tp0 = temp0;
		Vector3 dir0 = temp1 - temp0;
		const float scale = 1.f;
		const Vector3 tp1 = tp0Center + dir0.Normal() * dir0.Length() * scale;
		tp0 = tp0Center - (dir0.Normal() * dir0.Length() * scale);

		Vector3 tp1Center = (rect[0] + rect[1]) / 2.f;
		Vector3 dir1 = rect[0] - rect[1];
		const Vector3 tp3 = tp1Center + dir1.Normal() * dir1.Length() * scale;
		const Vector3 tp4 = tp1Center - (dir1.Normal() * dir1.Length() * scale);

		// 중점을 지나치면, 종료
		const Vector3 nd1 = (rect[3] - rect[1]).Normal();
		const float dot = nd1.DotProduct(d1);
		if (dot < 0.f)
			break;

		Vector2 out;
		if (GetIntersectPoint(Vector2(tp0.x, tp0.z), Vector2(tp1.x, tp1.z)
			, Vector2(tp3.x, tp3.z), Vector2(tp4.x, tp4.z), &out))
		{
			saveRoute = true;
			m_genPath.push_back(Vector2d(out.x, out.y));
		}
		else
		{
			gap /= 2.f;
			saveRoute = false;

			rect[0] = oldRect[0] + (d0 * gap);
			rect[1] = oldRect[1] + (d1 * gap);
			rect[2] = oldRect[2] - (d0 * gap);
			rect[3] = oldRect[3] - (d1 * gap);

			if (gap < 0.0003f)
			{
				m_genPath.push_back(Vector2d(center.x, center.z));
				break; // exit
			}
		}
	}

	return true;
}


void cRoute::Clear()
{
	m_path.clear();
	m_genPath.clear();
}
