
#include "stdafx.h"
#include <iostream>
#include <string>
using namespace std;

common::cMemoryPool2<65 * 65 * sizeof(float)> g_memPool65;
common::cMemoryPool2<67 * 67 * sizeof(float)> g_memPool67;
common::cMemoryPool2<256 * 256 * sizeof(float)> g_memPool256;
common::cMemoryPool2<258 * 258 * sizeof(float)> g_memPool258;
common::cMemoryPool3<graphic::cTexture, 512> g_memPoolTex;
int MAX_SCAN_LEVEL = 20;

struct sLocation {
	int lv;
	int x;
	int y;
};

class cDownloadListener : public gis::iDownloadFinishListener
{
	virtual void OnDownloadComplete(const gis::sDownloadData &data) override
	{
		cout << "Complete " << data.dataFile.c_str() << endl;
	}
};

// scan function
void scan_vworld(gis::cGeoDownloader &geoDownloader
	, const StrPath &mediaDir, queue<sLocation> &q);
void scan_arcgis(gis::cGeoDownloader &geoDownloader
	, const StrPath &mediaDir, queue<sLocation> &q);


int main(char argc, char *argv[])
{
	if (argc < 3)
	{
		cout << "server:{vworld, arcgis}, maxLv:number" << endl;
		return 0;
	}

	const string svr = argv[1];
	cout << "server= " << svr << endl;
	MAX_SCAN_LEVEL = min(MAX_SCAN_LEVEL, atoi(argv[2]));
	cout << "max level=" << MAX_SCAN_LEVEL << endl;
	Sleep(3000);

	cConfig config("carnavi_config.txt");

	gis::cGeoDownloader geoDownloader;
	cDownloadListener listener;
	geoDownloader.Create(config.GetString("apikey"), &listener);
	StrPath mediaDir = config.GetString("media_path", "D:\\media\\data");
	g_mediaDir = mediaDir;
	
	//ex) 6, 548, 225
	std::queue<sLocation> q;
	if (0)
	{
		// vworld
		q.push({ 5, 272, 113}); // 서울
		q.push({ 5, 273, 113 }); // 가평
		q.push({ 5, 274, 113 }); // 정선
		q.push({ 6, 545, 228}); // 철원
		q.push({ 6, 546, 228 }); // 평강
		q.push({ 6, 547, 228 }); // 금강
		q.push({ 6, 548, 228 }); // 고성
		q.push({ 5, 272, 112 }); // 서산
		q.push({ 5, 273, 112 }); // 대전
		q.push({ 5, 274, 112 }); // 안동
		q.push({ 6, 550, 225}); // 울진
		q.push({ 6, 550, 224 }); // 영덕
		q.push({ 7, 1105, 453}); // 울릉
		q.push({ 5, 272, 111 }); // 광주
		q.push({ 5, 273, 111 }); // 함양
		q.push({ 5, 274, 111 }); // 대구
		q.push({ 6, 550, 223 }); // 울산
		q.push({ 5, 272, 110 }); // 완도
		//q.push({ 5, 273, 110 }); // 고흥
		q.push({ 6, 546, 221}); // 고흥
		q.push({ 6, 547, 221 }); // 여수
		//q.push({ 5, 274, 110 }); // 거제
		q.push({ 6, 548, 221}); // 거제
		q.push({ 6, 544, 219}); // 제주1
		q.push({ 6, 545, 219 }); // 제주2
		q.push({ 6, 548, 225 }); //
	}

	if (1)
	{
		// arcgis world
		//q.push({ 0, 0, 0 }); 

		// arcgis korea
		q.push({ 8, 218, 157 });
		q.push({ 8, 219, 157 });
		//8, 217, 156
		q.push({ 9, 435, 313 });
		q.push({ 9, 435, 312 });
		q.push({ 8, 218, 156 });
		q.push({ 8, 219, 156 });
		q.push({ 9, 440, 312 });
		//8, 217, 155
		q.push({ 9, 435, 311 });
		q.push({ 9, 435, 310 });
		q.push({ 8, 218, 155 });
		q.push({ 8, 219, 155 });
		q.push({ 9, 440, 311 });
		q.push({ 9, 440, 310 });
		//8, 217, 154
		q.push({ 9, 435, 309 });
		q.push({ 9, 435, 308 });
		q.push({ 8, 218, 154 });
		q.push({ 8, 219, 154 });
		q.push({ 8, 217, 153 });
		q.push({ 8, 218, 153 });

		// uleug dokdo
		q.push({ 9, 442, 313 });
	}

	if (svr == "vworld")
	{
		while (!q.empty())
			scan_vworld(geoDownloader, mediaDir, q);
	}
	else if (svr == "arcgis")
	{
		while (!q.empty())
			scan_arcgis(geoDownloader, mediaDir, q);
	}
	else
	{
		cout << "error server name" << endl;
	}

	while (geoDownloader.m_requestIds.size() > 0)
		Sleep(1000);
}


// scan lv, x, ychild quad
// and then scan child quad recursivly
void scan_vworld(gis::cGeoDownloader &geoDownloader
	, const StrPath &mediaDir, queue<sLocation> &q)
{
	if (q.empty())
		return; // finish scan

	const sLocation loc = q.front(); q.pop();
	if (loc.lv >= MAX_SCAN_LEVEL)
		return; // finish scan

	const int lv = loc.lv;

	const int startX = loc.x << 1;
	const int endX = (loc.x + 1) << 1;
	const int startY = loc.y << 1;
	const int endY = (loc.y + 1) << 1;
	for (int x = startX; x < endX; ++x)
	{
		for (int y = startY; y < endY; ++y)
		{
			q.push({ lv + 1, x, y });

			// heightmap
			{
				const StrPath fileName = cHeightmap2::GetFileName(mediaDir, lv+1, x, y);
				const bool isFileExist = fileName.IsFileExist();
				if (!isFileExist) {
					cout << "request heightmap " << fileName.c_str() << endl;
					geoDownloader.DownloadFile(gis::eGeoServer::XDWORLD
						, lv+1, x, y,
						0, gis::eLayerName::DEM, fileName.c_str());
				}
			}

			// texture
			{
				const StrPath fileName = cTileTexture::GetFileName(mediaDir, lv + 1, x, y);
				const bool isFileExist = fileName.IsFileExist();
				if (!isFileExist) {
					cout << "request texture " << fileName.c_str() << endl;
					geoDownloader.DownloadFile(gis::eGeoServer::XDWORLD
						, lv + 1, x, y,
						0, gis::eLayerName::TILE, fileName.c_str());
				}
			}

			// poi base
			{
				const StrPath fileName = cPoiReader::GetFileName(mediaDir, lv + 1, x, y
					, gis::eLayerName::POI_BASE);
				const bool isFileExist = fileName.IsFileExist();
				if (!isFileExist) {
					cout << "request poi base" << fileName.c_str() << endl;
					geoDownloader.DownloadFile(gis::eGeoServer::XDWORLD
						, lv + 1, x, y,
						0, gis::eLayerName::POI_BASE, fileName.c_str());
				}
			}

			// poi bound
			{
				const StrPath fileName = cPoiReader::GetFileName(mediaDir, lv + 1, x, y
					, gis::eLayerName::POI_BOUND);
				const bool isFileExist = fileName.IsFileExist();
				if (!isFileExist) {
					cout << "request poi bound" << fileName.c_str() << endl;
					geoDownloader.DownloadFile(gis::eGeoServer::XDWORLD
						, lv + 1, x, y,
						0, gis::eLayerName::POI_BOUND, fileName.c_str());
				}
			}

			geoDownloader.UpdateDownload();
			if (geoDownloader.m_requestIds.size() > 100)
				Sleep(1000);
		}
	}
}


// scan lv, x, y child node
// and then scan child tree node recursivly
void scan_arcgis(gis::cGeoDownloader &geoDownloader
	, const StrPath &mediaDir, queue<sLocation> &q)
{
	if (q.empty())
		return; // finish scan

	const sLocation loc = q.front(); q.pop();
	if (loc.lv > MAX_SCAN_LEVEL)
		return; // finish scan

	// heightmap
	{
		const StrPath fileName = gis2::cHeightmap3::GetFileName(mediaDir, loc.lv, loc.x, loc.y);
		const bool isFileExist = fileName.IsFileExist();
		if (!isFileExist) {
			cout << "request heightmap " << fileName.c_str() << endl;
			geoDownloader.DownloadFile(gis::eGeoServer::ARCGIS
				, loc.lv, loc.x, loc.y,
				0, gis::eLayerName::DEM, fileName.c_str());
		}
	}

	// texture
	{
		const StrPath fileName = cTileTexture::GetFileName2(mediaDir, loc.lv, loc.x, loc.y);
		const bool isFileExist = fileName.IsFileExist();
		if (!isFileExist) {
			cout << "request texture " << fileName.c_str() << endl;
			geoDownloader.DownloadFile(gis::eGeoServer::ARCGIS
				, loc.lv, loc.x, loc.y,
				0, gis::eLayerName::TILE, fileName.c_str());
		}
	}

	geoDownloader.UpdateDownload();
	if (geoDownloader.m_requestIds.size() > 100)
		Sleep(geoDownloader.m_requestIds.size() * 10);

	const int startX = loc.x << 1;
	const int endX = (loc.x + 1) << 1;
	const int startY = loc.y << 1;
	const int endY = (loc.y + 1) << 1;
	for (int x = startX; x < endX; ++x)
		for (int y = startY; y < endY; ++y)
			q.push({ loc.lv + 1, x, y });
}
