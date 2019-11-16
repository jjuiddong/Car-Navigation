
#include "stdafx.h"
#include "webdownload.h"

using namespace gis;


// 다운로드 파일이름을 생성해 리턴한다.
StrPath gis::GetDownloadFileName(const StrPath &mediaDir, const sDownloadData &dnData)
{
	StrPath dstFileName;
	dstFileName.Format("%s\\%d\\%04d\\%04d_%04d"
		, mediaDir.c_str()
		, dnData.level
		, dnData.yLoc
		, dnData.yLoc
		, dnData.xLoc);

	switch (dnData.layer)
	{
	case eLayerName::DEM: dstFileName += ".bil"; break;
	case eLayerName::TILE: dstFileName += ".dds"; break;
	case eLayerName::POI_BASE: dstFileName += ".poi_base"; break;
	case eLayerName::POI_BOUND: dstFileName += ".poi_bound"; break;
	case eLayerName::FACILITY_BUILD: dstFileName += ".facility_build"; break;
	case eLayerName::FACILITY_BUILD_GET: 
		dstFileName.Format("%s\\%d\\%04d\\%s"
			, mediaDir.c_str()
			, dnData.level
			, dnData.yLoc
			, dnData.dataFile.c_str());
		break;
	case eLayerName::FACILITY_BUILD_GET_JPG:
		dstFileName.Format("%s\\%d\\%04d\\%s"
			, mediaDir.c_str()
			, dnData.level
			, dnData.yLoc
			, dnData.dataFile.c_str());
		break;
	default: assert(0); break;
	}

	return dstFileName;
}
