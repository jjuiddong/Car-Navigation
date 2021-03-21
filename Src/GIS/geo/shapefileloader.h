//
// 2020-01-12, jjuiddong
// shapefile loader
//	- The shapefile format is a geospatial vector data format for 
//	  geographic information system (GIS) software. 
//	  It is developed and regulated by Esri as a mostly open 
//    specification for data interoperability among Esri and 
//    other GIS software products.
//  - https://en.wikipedia.org/wiki/Shapefile
//
//	- shapefile format
//		- https://www.esri.com/library/whitepapers/pdfs/shapefile.pdf
//
#pragma once


class cShapefileLoader
{
public:
	struct sPoint
	{
		double x; // longitude
		double y; // latitude
	};

	struct sPolyLine
	{
		double box[4];
		int numParts;
		int numPoints;
		int *parts;
		int *points;
	};

	struct sPolygon
	{
		double box[4];
		int numParts;
		int numPoints;
		int *parts;
		sPoint *points;
	};

	struct sRecord
	{
		int num;
	};

	cShapefileLoader();
	virtual ~cShapefileLoader();

	bool Read(const char *fileName);


public:
	// no remove container, remove external
	vector<sPolygon*> m_polylines;
	vector<sPolygon*> m_polygons;
};
