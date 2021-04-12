//
// 2021-04-09, jjuiddong
// googlemap download
// 
#pragma once

namespace gis { class cGeoDownloader; }


class cTaskGoogleDownload : public common::cTask
	, public common::cMemoryPool4<cTaskGoogleDownload>
{
public:
	cTaskGoogleDownload(const int id
		, gis::cGeoDownloader *geoDownloader
		, const gis::sDownloadData &dnData);
	virtual ~cTaskGoogleDownload();

	void SetParameter(gis::cGeoDownloader *geoDownloader
		, const gis::sDownloadData &dnData);

	virtual eRunResult::Enum Run(const double deltaSeconds);

	virtual void Clear() override;


public:
	gis::cGeoDownloader *m_geoDownloader; // reference
	gis::sDownloadData m_dnData;
};
