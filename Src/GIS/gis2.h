//
// 2021-04-03, jjuiddong
// gis library
//	- arcgis, bingmap gis system
//
#pragma once


namespace gis2
{

	// calc geo system
	Vector2d Pos2WGS84(const Vector3 &relPos);
	Vector3 WGS842Pos(const Vector2d &lonLat);
	Vector3 GetGlobalPos(const Vector3 &relPos);
	Vector3 GetRelationPos(const Vector3 &globalPos);

	// calc quadtree node
	std::tuple<int, int, int> GetNodeLevel(const sRectf &rect);
	std::pair<int, int> GetNodeLocation(const sRectf &rect, const int level);
	sRectf GetNodeRect(const int level, const int x, const int y);

}
