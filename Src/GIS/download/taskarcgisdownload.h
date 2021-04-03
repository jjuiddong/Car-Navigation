//
// 2021-03-30, jjuiddong
// arcgis download
// 
#pragma once

namespace gis { class cGeoDownloader; }


class cTaskArcGisDownload : public common::cTask
	, public common::cMemoryPool4<cTaskArcGisDownload >
{
public:
	cTaskArcGisDownload(const int id
		, gis::cGeoDownloader *geoDownloader
		, const gis::sDownloadData &dnData);
	virtual ~cTaskArcGisDownload();

	void SetParameter(gis::cGeoDownloader *geoDownloader
		, const gis::sDownloadData &dnData);

	virtual eRunResult::Enum Run(const double deltaSeconds);

	virtual void Clear() override;


public:
	gis::cGeoDownloader *m_geoDownloader; // reference
	gis::sDownloadData m_dnData;
};
