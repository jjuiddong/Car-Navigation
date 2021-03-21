//
// 2018-06-21, jjuiddong
// task web download
// 
#pragma once

class cQuadTileManager;
namespace gis { class cGeoDownloader; }


class cTaskWebDownload : public common::cTask
						, public common::cMemoryPool4<cTaskWebDownload>
{
public:
	cTaskWebDownload();
	cTaskWebDownload(gis::cGeoDownloader *webDownloader
		, cQuadTileManager *tileMgr
		, const gis::sDownloadData &dnData);
	virtual ~cTaskWebDownload();

	void SetParameter(gis::cGeoDownloader *webDownloader
		, cQuadTileManager *tileMgr
		, const gis::sDownloadData &dnData);

	virtual eRunResult::Enum Run(const double deltaSeconds);

	virtual void Clear() override;


public:
	gis::cGeoDownloader *m_webDownloader;
	cQuadTileManager *m_tileMgr;
	gis::sDownloadData m_dnData;
};
