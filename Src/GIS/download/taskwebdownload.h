//
// 2018-06-21, jjuiddong
// task web download
// 
#pragma once

namespace gis { class cGeoDownloader; }


class cTaskWebDownload : public common::cTask
						, public common::cMemoryPool4<cTaskWebDownload>
{
public:
	cTaskWebDownload(const int id
		, gis::cGeoDownloader *geoDownloader
		, const gis::sDownloadData &dnData);
	virtual ~cTaskWebDownload();

	void SetParameter(gis::cGeoDownloader *geoDownloader
		, const gis::sDownloadData &dnData);

	virtual eRunResult::Enum Run(const double deltaSeconds);

	virtual void Clear() override;


public:
	gis::cGeoDownloader *m_geoDownloader; // reference
	gis::sDownloadData m_dnData;
};
