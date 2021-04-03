
#include "stdafx.h"
#include "gis2.h"

using namespace gis2;

namespace {
	// relation coordinate origin position
	const static int g_orgLv = 8;
	const static int g_orgX = 219;
	const static int g_orgY = 155;
}


// return longitude, latitude correspond relation position
// ignore y (height)
// https://en.wikipedia.org/wiki/Mercator_projection
// return: lon-lat
Vector2d gis2::Pos2WGS84(const Vector3 &relPos)
{
	const Vector3 globalPos = GetGlobalPos(relPos);
	const double size = (double)(1 << MAX_QTREE_LEVEL);
	const double mapWidth = size;
	const double mapHeight = mapWidth;
	const double radius = mapWidth / (2.0 * MATH_PI2);

	const double x = globalPos.x - (mapWidth / 2.0); // from prime meridian
	const double y = globalPos.z - (mapHeight / 2.0); // from equator
	double lat = 2.0 * atan(exp(y / radius)) - MATH_PI / 2.0;
	lat = gis::RadToDeg(lat);
	double lon = x / radius;
	lon = gis::RadToDeg(lon);
	return Vector2d(lon, lat);
}


// return 3d pos(relation space) correspond longitude/latitude
// https://en.wikipedia.org/wiki/Mercator_projection
// https://github.com/blaurt/map-app-v1
// return relation coordinate 3d pos
Vector3 gis2::WGS842Pos(const Vector2d &lonLat)
{
	const double lon = lonLat.x;
	const double lat = lonLat.y;

	const double size = (double)(1 << MAX_QTREE_LEVEL);
	const double mapWidth = size;
	const double mapHeight = mapWidth;
	const double FE = 180.0;
	const double radius = mapWidth / (2.0 * MATH_PI2);
	const double lonRad = gis::DegToRad(lon + FE);
	const double latRad = gis::DegToRad(lat);

	const double yFromEquator = radius * log(tan(MATH_PI2 / 4.0 + latRad / 2.0));
	const double y = mapHeight / 2.0 - yFromEquator;

	const double x = lonRad * radius;
	const double xx = x;
	const double yy = size - y;
	const Vector3 gpos((float)xx, 0, (float)yy); // global space
	return gis2::GetRelationPos(gpos); // relation space
}


// return node level, x, y correspond rect
std::tuple<int, int, int> gis2::GetNodeLevel(const sRectf &rect)
{
	if ((rect.right < 0) || (rect.bottom < 0))
		return std::make_tuple(-1, 0, 0);

	int x1 = (int)(rect.left);
	int y1 = (int)(rect.top);

	int xResult = x1 ^ ((int)(rect.right));
	int yResult = y1 ^ ((int)(rect.bottom));

	int nodeLevel = MAX_QTREE_LEVEL;
	int shiftCount = 0;

	while (xResult + yResult != 0)
	{
		xResult >>= 1;
		yResult >>= 1;
		nodeLevel--;
		shiftCount++;
	}

	x1 >>= shiftCount;
	y1 >>= shiftCount;
	return std::make_tuple(nodeLevel, x1, y1);
}


// return nodeLevel, x, y correspond rect and level
std::pair<int, int> gis2::GetNodeLocation(
	const sRectf &rect, const int level)
{
	const auto result = GetNodeLevel(rect);
	int lv = std::get<0>(result);
	int x = std::get<1>(result);
	int y = std::get<2>(result);

	if (lv < level)
		return std::make_pair(-1, -1); // error occurred!!

	// find level, x, y
	while (lv != level)
	{
		x >>= 1;
		y >>= 1;
		--lv;
	}
	return std::make_pair(x, y);
}


// return Relation Coordinate
sRectf gis2::GetNodeRect(const int level, const int x, const int y)
{
	// global position
	sRectf rect;
	const float size = (float)(1 << (MAX_QTREE_LEVEL - level));
	rect = sRectf::Rect(x * size, y * size, size, size);

	// origin position
	const float size2 = (float)(1 << (MAX_QTREE_LEVEL - g_orgLv));
	const Vector2 orgPos(g_orgX * size2, g_orgY * size2);

	// calc relation position
	rect.Offset(-orgPos.x, -orgPos.y);
	return rect;
}


// convert relation coordinate to global coordinate
Vector3 gis2::GetGlobalPos(const Vector3 &relPos)
{
	const float size = (float)(1 << (MAX_QTREE_LEVEL - g_orgLv));
	const float offsetX = size * g_orgX;
	const float offsetY = size * g_orgY;
	return Vector3(relPos.x + offsetX, relPos.y, relPos.z + offsetY);
}


// convert relation coordinate to global coordinate
Vector3 gis2::GetRelationPos(const Vector3 &globalPos)
{
	const float size = (float)(1 << (MAX_QTREE_LEVEL - g_orgLv));
	const int x = g_orgX;
	const int y = g_orgY;
	const float offsetX = size * x;
	const float offsetY = size * y;
	return Vector3(globalPos.x - offsetX, globalPos.y, globalPos.z - offsetY);
}

