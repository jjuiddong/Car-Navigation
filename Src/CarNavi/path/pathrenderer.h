//
// 2019-08-10, jjuiddong
// path renderer
//	- create from cPathFile (wgs84 pos)
//	- create from 3d position file
//		- *.pos3d
//
//	- pos 3d file format
//		- pos size
//		- pos array (pos: Vector3)
//
#pragma once


class cPathRenderer : public graphic::cNode
{
public:
	cPathRenderer();
	virtual ~cPathRenderer();

	bool Create(graphic::cRenderer &renderer, cTerrainQuadTree &terrain, const cPathFile &path
		, const StrPath &pos3DFileName="");
	bool Create(graphic::cRenderer &renderer, const StrPath &pos3DFileName);
	virtual bool Update(graphic::cRenderer &renderer, const float deltaSeconds) override;
	virtual bool Render(graphic::cRenderer &renderer, const XMMATRIX &parentTm = graphic::XMIdentity, const int flags = 1) override;
	virtual void Clear() override;


protected:
	bool WritePos3DFile(const StrPath &fileName);


public:
	StrPath m_fileName;
	graphic::cColor m_color;
	graphic::cDbgLineStrip m_lineList;
};
