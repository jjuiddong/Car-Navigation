
#include "stdafx.h"
#include <iostream>
using namespace std;

cRoot g_root;
common::cMemoryPool2<65 * 65 * sizeof(float)> g_memPool65;
common::cMemoryPool2<67 * 67 * sizeof(float)> g_memPool67;
common::cMemoryPool2<256 * 256 * sizeof(float)> g_memPool256;
common::cMemoryPool2<258 * 258 * sizeof(float)> g_memPool258;
common::cMemoryPool3<graphic::cTexture, 512> g_memPoolTex;
int MAX_SCAN_LEVEL = 14;

struct sLocation {
	int lv;
	int xLoc;
	int yLoc;
};

class cDownloadListener : public gis::iDownloadFinishListener
{
	virtual void OnDownloadComplete(const gis::sDownloadData &data) override
	{
		cout << "Complete " << data.dataFile.c_str() << endl;
	}
};

// scan lv, xLoc, yLoc child quad
// and then scan child quad recursivly
void scan(cQuadTileManager &tileMgr, cDownloadListener &listener,
	const StrPath &mediaDir, queue<sLocation> &q);

int main(char argc, char *argv[])
{
	if (argc < 2)
	{
		cout << "maxLv" << endl;
		return 0;
	}

	MAX_SCAN_LEVEL = min(16, atoi(argv[1]));
	cout << "max level=" << MAX_SCAN_LEVEL << endl;
	Sleep(3000);

	cConfig config("carnavi_config.txt");

	cQuadTileManager tileMgr;
	cDownloadListener listener;
	tileMgr.m_geoDownloader.Create(config.GetString("apikey"), false);
	StrPath mediaDir = config.GetString("media_path", "D:\\media\\data");
	g_mediaDir = mediaDir;
	
	//ex) 6, 548, 225
	std::queue<sLocation> q;
	q.push({ 5, 272, 113}); // ����
	q.push({ 5, 273, 113 }); // ����
	q.push({ 5, 274, 113 }); // ����
	q.push({ 6, 545, 228}); // ö��
	q.push({ 6, 546, 228 }); // ��
	q.push({ 6, 547, 228 }); // �ݰ�
	q.push({ 6, 548, 228 }); // ��
	q.push({ 5, 272, 112 }); // ����
	q.push({ 5, 273, 112 }); // ����
	q.push({ 5, 274, 112 }); // �ȵ�
	q.push({ 6, 550, 225}); // ����
	q.push({ 6, 550, 224 }); // ����
	q.push({ 7, 1105, 453}); // �︪
	q.push({ 5, 272, 111 }); // ����
	q.push({ 5, 273, 111 }); // �Ծ�
	q.push({ 5, 274, 111 }); // �뱸
	q.push({ 6, 550, 223 }); // ���
	q.push({ 5, 272, 110 }); // �ϵ�
	//q.push({ 5, 273, 110 }); // ����
	q.push({ 6, 546, 221}); // ����
	q.push({ 6, 547, 221 }); // ����
	//q.push({ 5, 274, 110 }); // ����
	q.push({ 6, 548, 221}); // ����
	q.push({ 6, 544, 219}); // ����1
	q.push({ 6, 545, 219 }); // ����2
	
	q.push({ 6, 548, 225 }); //

	while (!q.empty())
		scan(tileMgr, listener, mediaDir, q);

	while (tileMgr.m_geoDownloader.m_requestIds.size() > 0)
		Sleep(1000);
}


// scan lv, xLoc, yLoc child quad
// and then scan child quad recursivly
void scan(cQuadTileManager &tileMgr, cDownloadListener &listener,
	const StrPath &mediaDir, queue<sLocation> &q)
{
	if (q.empty())
		return; // finish scan

	const sLocation loc = q.front(); q.pop();
	const int lv = loc.lv;
	const int xLoc = loc.xLoc;
	const int yLoc = loc.yLoc;
	if (lv >= MAX_SCAN_LEVEL)
		return; // finish scan

	const int startX = xLoc << 1;
	const int endX = (xLoc + 1) << 1;
	const int startY = yLoc << 1;
	const int endY = (yLoc + 1) << 1;
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
					tileMgr.m_geoDownloader.DownloadFile(lv+1, x, y,
						0, gis::eLayerName::DEM, tileMgr, &listener, fileName.c_str());
				}
			}

			// texture
			{
				const StrPath fileName = cTileTexture::GetFileName(mediaDir, lv + 1, x, y);
				const bool isFileExist = fileName.IsFileExist();
				if (!isFileExist) {
					cout << "request texture " << fileName.c_str() << endl;
					tileMgr.m_geoDownloader.DownloadFile(lv + 1, x, y,
						0, gis::eLayerName::TILE, tileMgr, &listener, fileName.c_str());
				}
			}

			// poi base
			{
				const StrPath fileName = cPoiReader::GetFileName(mediaDir, lv + 1, x, y
					, gis::eLayerName::POI_BASE);
				const bool isFileExist = fileName.IsFileExist();
				if (!isFileExist) {
					cout << "request poi base" << fileName.c_str() << endl;
					tileMgr.m_geoDownloader.DownloadFile(lv + 1, x, y,
						0, gis::eLayerName::POI_BASE, tileMgr, &listener, fileName.c_str());
				}
			}

			// poi bound
			{
				const StrPath fileName = cPoiReader::GetFileName(mediaDir, lv + 1, x, y
					, gis::eLayerName::POI_BOUND);
				const bool isFileExist = fileName.IsFileExist();
				if (!isFileExist) {
					cout << "request poi bound" << fileName.c_str() << endl;
					tileMgr.m_geoDownloader.DownloadFile(lv + 1, x, y,
						0, gis::eLayerName::POI_BOUND, tileMgr, &listener, fileName.c_str());
				}
			}

			tileMgr.m_geoDownloader.UpdateDownload();
			if (tileMgr.m_geoDownloader.m_requestIds.size() > 100)
				Sleep(1000);
		}
	}
}
