//
// 2018-05-01, jjuiddong
// geometry data download from geo server *.bil, *.dds, *.dat, .. etc
//
// 2019-05-18
//	- download using thread pool
//
// 2020-09-10
//	- apikey external file
//
// 2021-03-30
//	- refactoring
//	- arcgis
//	- googlemap (04-09)
//
#pragma once


namespace gis
{
	enum class eGeoServer {
		XDWORLD 
		, ARCGIS
		, GOOGLEMAP
	};

	class cGeoDownloader
	{
	public:
		friend class cTaskWebDownload;
		friend class cTaskArcGisDownload;
		friend class cTaskGoogleDownload;

		// keytype:  filename hash + (id + layer * 100) + cQuadTree::MakeKey()
		typedef std::tuple<int, int, __int64> keytype;


		cGeoDownloader();
		virtual ~cGeoDownloader();

		bool Create(const string &apiKey, iDownloadFinishListener *listener);
		bool DownloadFile(const eGeoServer svrType
			, const int level, const int x, const int y
			, const int idx
			, const eLayerName::Enum type
			, const char *dataFileName = NULL
		);
		bool CancelDownload(const int level, const int x, const int y
			, const int idx
			, const eLayerName::Enum type
			, const char *dataFileName = NULL);
		void UpdateDownload();
		void Clear();


	protected:
		keytype MakeKey(const sDownloadData &dnData);
		bool CompleteDownload(const sDownloadData &dnData);
		bool FailDownload(const sDownloadData &dnData);


	public:
		Str64 m_apiKey;
		bool m_isOfflineMode;
		iDownloadFinishListener *m_listener; // download event listener
		CriticalSection m_cs;
		vector<sDownloadData> m_complete; // complete download file list

		// check duplicate request
		set<keytype> m_requestIds;
		map<keytype, int> m_taskIds; // key: download key, value: task id
									 // store task id to remove
		__int64 m_totalDownloadFileSize;
		common::cTPSemaphore m_tpDownloader;
	};

}
