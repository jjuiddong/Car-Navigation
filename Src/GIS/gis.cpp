
#include "stdafx.h"
#include "gis.h"
#include "quadtree.h"

using namespace gis;

// 레벨당 위경도 각도
// index = level
static const double deg[16] = {
	36.f  // 0 level
	, 36.f / 2.f // 1 level
	, 36.f / 2.f / 2.f // 2 level
	, 36.f / 2.f / 2.f / 2.f // 3 level
	, 36.f / 2.f / 2.f / 2.f / 2.f // 4 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f // 5 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 6 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 7 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 8 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 9 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 10 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 11 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 12 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 13 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 14 level
	, 36.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f / 2.f // 15 level
};

static const int g_offsetLv = 7;
static const int g_offsetXLoc = 1088;
static const int g_offsetYLoc = 442;



// return Tile Left-Top WGS84 Location
// xLoc : col, 경도
// yLoc : row, 위도
//
// 0 level, row=5, col=10
//	- latitude : 36 degree / row 1
//	- longitude : 36 degrre / col 1
//
// return: .x = longitude (경도)
//		   .y = latitude (위도)
Vector2d gis::TileLoc2WGS84(const int level, const int xLoc, const int yLoc)
{
	Vector2d lonLat;
	lonLat.x = deg[level] * xLoc - 180.f;
	lonLat.y = deg[level] * yLoc - 90.f;
	return lonLat;
}


Vector2d gis::TileLoc2WGS84(const sTileLoc &loc)
{
	return TileLoc2WGS84(loc.level, loc.xLoc, loc.yLoc);
}


sTileLoc gis::WGS842TileLoc(const Vector2d &lonLat)
{
	assert(0);
	return {};
}


// Convert 3D Position to longitude, raditude
// return: .x = longitude (경도)
//		   .y = latitude (위도)
//
// leftBottomLonLat, rightTopLonLat:
//   +--------* (Right Top Longitude Latitude)
//   |        |
//   |        |
//   |        |
//   *--------+ 
//	(Left Bottom Longitude Latitude)
//
// rect: VWorld 3D Coordinate system
//       Ground X-Z Plane Rect
//
//  /\ Z 
//  |
//  | 
//  |
//  ---------> X 
//  Ground = X-Z Plane
//
Vector2d gis::Pos2WGS84(const Vector3 &pos
	, const Vector2d &leftBottomLonLat, const Vector2d &rightTopLonLat
	, const sRectf &rect)
{
	Vector3 p = Vector3(pos.x - rect.left, pos.y, pos.z - rect.top);
	p.x /= rect.Width();
	p.z /= rect.Height();

	Vector2d ret;
	ret.x = common::lerp(leftBottomLonLat.x, rightTopLonLat.x, (double)p.x);
	ret.y = common::lerp(leftBottomLonLat.y, rightTopLonLat.y, (double)p.z);
	return ret;
}


// Convert longitude, latitude to 3D Position (Global Position)
Vector3 gis::WGS842Pos(const Vector2d &lonLat)
{
	const double size = 1 << 16;
	const double x = (lonLat.x + 180.f) * (size / 36.f);
	const double z = (lonLat.y + 90.f) * (size / 36.f);
	return Vector3((float)x, 0, (float)z);
}


Vector3 gis::GetRelationPos(const Vector3 &globalPos)
{
	const float size = (1 << (16 - g_offsetLv));
	const int xLoc = g_offsetXLoc;
	const int yLoc = g_offsetYLoc;
	const float offsetX = size * xLoc;
	const float offsetY = size * yLoc;
	return Vector3(globalPos.x - offsetX, globalPos.y, globalPos.z - offsetY);
}


// meter -> 3D unit
// 지구 적도 둘레, 40030000.f,  40,030 km
// 경도축으로 10등분 한 크기를 (1 << cQuadTree<int>::MAX_LEVEL) 로 3D로 출력한다.
float gis::Meter23DUnit(const float meter)
{
	static const float rate = (float)(1 << cQuadTree<int>::MAX_LEVEL) / (40030000.f * 0.1f);
	return meter * rate;
}


// GPRMC 문자열로 위경도를 리턴한다.
//$GPRMC, 103906.0, A, 3737.084380, N, 12650.121679, E, 0.0, 0.0, 190519, 6.1, W, A * 15
// https://drkein.tistory.com/113
// lat: 37 + 37.084380/60
// lon: 126 + 50.121679/60
// date: 
//		- DDMMYY
//		- year: 2019
//		- month: 05
//		- day : 19
//		HHMMSS.SS
//		- hour: 10
//		- min: 39
//		- sec: 6.0
// return : x=longitude, y=latitude
bool gis::GetGPRMCLonLat(const Str512 &gprmc, OUT sGPRMC &out)
{
	if (gprmc.size() < 6)
		return false;
	
	if (strncmp(gprmc.m_str, "$GPRMC", 6))
		return false;

	vector<string> strs;
	common::tokenizer(gprmc.c_str(), ",", "", strs);

	if (strs.size() < 7)
		return false;

	if (strs[2] != "A")
		return false; // invalid data

	if (strs[4] != "N")
		return false;
	
	if (strs[6] != "E")
		return false;

	if (strs[3].size() < 2)
		return false;

	Vector2d lonLat;
	const float lat1 = (float)atof(strs[3].substr(0, 2).c_str());
	const float lat2 = (float)atof(strs[3].substr(2).c_str());
	if (lat2 == 0.f)
		return false;
	lonLat.y = lat1 + (lat2 / 60.f);

	if (strs[5].size() < 3)
		return false;
	const float lon1 = (float)atof(strs[5].substr(0, 3).c_str());
	const float lon2 = (float)atof(strs[5].substr(3).c_str());
	if (lon2 == 0.f)
		return false;
	lonLat.x = lon1 + (lon2 / 60.f);

	out.available = true;
	out.lonLat = lonLat;
	out.speed = 0.f;
	out.angle = 0.f;
	out.north = 0;
	if (strs.size() >= 8)
		out.speed = (float)atof(strs[7].c_str()) * 1.8f;
	if (strs.size() >= 9)
		out.angle = (float)atof(strs[8].c_str());
	if (strs.size() >= 11)
		out.north = (float)atof(strs[10].c_str());

	// date
	out.date = 0;
	{
		// hhmmss.ss
		int h = 0, m = 0, s = 0, mm = 0;
		const char *p = strs[1].c_str();
		if (*p)
			h = (int)(*p - '0') * 10;
		++p;
		if (*p)
			h += (int)(*p - '0') * 1;
		++p;
		if (*p)
			m = (int)(*p - '0') * 10;
		++p;
		if (*p)
			m += (int)(*p - '0') * 1;
		++p;
		if (*p)
			s = (int)(*p - '0') * 10;
		++p;
		if (*p)
			s += (int)(*p - '0') * 1;

		++p;
		if (*p && (*p == '.'))
		{
			++p;
			if (*p)
				mm = (int)(*p - '0') * 100;
			++p;
			if (*p)
				mm += (int)(*p - '0') * 10;
		}

		out.date += (uint64)h * 10000000;
		out.date += (uint64)m * 100000;
		out.date += (uint64)s * 1000;
		out.date += (uint64)mm * 1;
	}

	if (strs.size() >= 10)
	{
		// DDMMYY
		int Y = 0, M = 0, D = 0;
		const char *p = strs[9].c_str();
		if (*p)
			D = (int)(*p - '0') * 10;
		++p;
		if (*p)
			D += (int)(*p - '0') * 1;
		++p;
		if (*p)
			M = (int)(*p - '0') * 10;
		++p;
		if (*p)
			M += (int)(*p - '0') * 1;
		++p;
		if (*p)
			Y = (int)(*p - '0') * 10;
		++p;
		if (*p)
			Y += (int)(*p - '0') * 1;

		out.date += (uint64)Y * 10000000000000 + 20000000000000000;
		out.date += (uint64)M * 100000000000;
		out.date += (uint64)D * 1000000000;
	}

	return true;
}


// GPATT 문자열로 위경도를 리턴한다.
// $GPATT, 37.939707, 126.836799, 44.3, 143.2, 7.3, 6.6, *71
bool gis::GetGPATTLonLat(const Str512 &gpatt, OUT sGPRMC &out)
{
	if (gpatt.size() < 6)
		return false;

	if (strncmp(gpatt.m_str, "$GPATT", 6))
		return false;

	vector<string> strs;
	common::tokenizer(gpatt.c_str(), ",", "", strs);

	if (strs.size() < 3)
		return false;

	ZeroMemory(&out, sizeof(out));
	out.available = true;
	out.lonLat.y = atof(strs[1].c_str());
	out.lonLat.x = atof(strs[2].c_str());
	
	// 가끔씩 데이타가 깨져서 들어오는 경우가 있음.
	if (out.lonLat.x <= 30.f)
		return false;
	if (out.lonLat.y <= 10.f)
		return false;

	return true;
}
