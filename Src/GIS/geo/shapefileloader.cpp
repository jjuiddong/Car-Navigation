
#include "stdafx.h"
#include "shapefileloader.h"


cShapefileLoader::cShapefileLoader()
{
}

cShapefileLoader::~cShapefileLoader()
{
}


// read *.shp file
//	- shapefile format
//		- https://www.esri.com/library/whitepapers/pdfs/shapefile.pdf
bool cShapefileLoader::Read(const char *fileName)
{
	struct sShapeHeader {
		int code;
		int dummy1[5];
		int length;
		int version;
		int shapetype;
		double dummy2[5];
	};
	struct sShapeRecord {
		int num;
		int length;
	};

	std::ifstream ifs(fileName, std::ios::binary);
	if (!ifs.is_open())
		return false;

	char headerBuff[100];
	ifs.read(headerBuff, 100);

	// _byteswap_ulong : bigendian -> littleendian
	sShapeHeader *header = (sShapeHeader*)headerBuff;
	header->code = _byteswap_ulong(header->code);
	header->length = _byteswap_ulong(header->length);

	while (!ifs.eof())
	{
		sShapeRecord record;
		ifs.read((char*)&record, sizeof(record));
		record.num = _byteswap_ulong(record.num);
		record.length = _byteswap_ulong(record.length);

		int shapeType = 0;
		ifs.read((char*)&shapeType, sizeof(shapeType));

		switch (shapeType)
		{
		case 0: break; // null shape
		case 1: // point record
		{
			sPoint point;
			ifs.read((char*)&point, sizeof(point));
		}
		break;

		case 3: // polyline record
		case 5: // polygon record
		{
			sPolygon *polygon = new sPolygon;
			// box, numParts, numPoints
			ifs.read((char*)polygon, sizeof(double)*4 + sizeof(int)*2);
			if (polygon->numParts <= 0)
				goto $error;

			polygon->parts = new int[polygon->numParts];
			ifs.read((char*)polygon->parts, sizeof(int) * polygon->numParts);

			polygon->points = new sPoint[polygon->numPoints];
			ifs.read((char*)polygon->points, sizeof(sPoint) * polygon->numPoints);

			if (shapeType == 3)
				m_polylines.push_back(polygon);
			else if (shapeType == 5)
				m_polygons.push_back(polygon);
		}
		break;

		case 8: // multpoint record
			break;
		default:
			break;
		}
	}

	return true;

$error:
	return false;
}
