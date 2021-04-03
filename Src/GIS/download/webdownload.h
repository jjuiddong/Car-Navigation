//
// 2018-06-21, jjuiddong
// webdownload global header
//
#pragma once


namespace gis
{
	struct eLayerName {
		enum Enum {
			DEM
			, TILE
			, POI_BASE
			, POI_BOUND
			, FACILITY_BUILD
			, FACILITY_BUILD_GET  // 3d mesh
			, FACILITY_BUILD_GET_JPG // texture
			, DEM_CPY
			, MAX_LAYERNAME
		};
	};

	struct sDownloadData
	{
		eLayerName::Enum layer;
		int level;
		int x;
		int y;
		int idx; // *.xdo file load
		common::Str64 dataFile;
	};

	StrPath GetDownloadFileName(const StrPath &mediaDir, const sDownloadData &dnData);

	interface iDownloadFinishListener {
		virtual void OnDownloadComplete(const sDownloadData &data) = 0;
	};

}
