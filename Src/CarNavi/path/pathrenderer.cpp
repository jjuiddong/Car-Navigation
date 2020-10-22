
#include "stdafx.h"
#include "pathrenderer.h"

using namespace graphic;


cPathRenderer::cPathRenderer()
	: cNode(common::GenerateId(), "Path", eNodeType::MODEL)
	, m_color(Vector4(1.f, 1.f, 1.f, 0.8f))
{
}

cPathRenderer::~cPathRenderer()
{
}


// path정보로 mesh를 만든다.
// pos3DFileName : 파일명이 있다면, 3DPos 포맷으로 파일을 저장한다.
bool cPathRenderer::Create(graphic::cRenderer &renderer, cTerrainQuadTree &terrain
	, const cPathFile &path
	, const StrPath &pos3DFileName //= ""
)
{
	Clear();

	m_lineList.Create(renderer, path.m_table.size(), m_color);
	for (auto &row : path.m_table)
	{
		const Vector3 pos = terrain.Get3DPosPrecise(renderer, row.lonLat);
		m_lineList.AddPoint(renderer, pos, false);
	}
	m_lineList.UpdateBuffer(renderer);

	if (!pos3DFileName.empty())
		WritePos3DFile(pos3DFileName);

	m_fileName = pos3DFileName;
	m_lineList.m_points.clear(); // 불필요한 정보는 제거한다.
	m_lineList.m_points.shrink_to_fit();

	return true;
}


// 3d position 정보를 가진 파일로 데이타를 읽은 후, mesh를 만든다.
bool cPathRenderer::Create(graphic::cRenderer &renderer, const StrPath &pos3DFileName)
{
	std::ifstream ifs(pos3DFileName.c_str(), std::ios::binary);
	if (!ifs.is_open())
		return false;

	Clear();

	uint size = 0;
	ifs.read((char*)&size, sizeof(size));
	if (size <= 0)
		return false;

	m_lineList.m_points.resize(size);
	ifs.read((char*)&m_lineList.m_points[0], sizeof(Vector3)*size);

	if (!m_lineList.Create(renderer, size, m_color))
		return false;

	m_lineList.m_pointCount = size;
	m_lineList.UpdateBuffer(renderer);

	m_fileName = pos3DFileName;
	m_lineList.m_points.clear(); // 불필요한 정보는 제거한다.
	m_lineList.m_points.shrink_to_fit();

	return true;
}


// m_lineList에 저장된 위치정보를 파일에 저장한다.
// format: binary
//	- size (4byte)
//	- pos array (pos: Vector3)
bool cPathRenderer::WritePos3DFile(const StrPath &fileName)
{
	std::ofstream ofs(fileName.c_str(), std::ios::binary);
	if (!ofs.is_open())
		return false;

	const uint size = m_lineList.m_points.size();
	ofs.write((char*)&size, sizeof(size));
	for (auto &pos : m_lineList.m_points)
		ofs.write((char*)&pos, sizeof(pos));
	return true;
}


bool cPathRenderer::Update(cRenderer &renderer, const float deltaSeconds)
{

	return true;
}


bool cPathRenderer::Render(cRenderer &renderer
	, const XMMATRIX &parentTm //= XMIdentity
	, const int flags //= 1
)
{
	CommonStates states(renderer.GetDevice());
	renderer.GetDevContext()->OMSetDepthStencilState(states.DepthNone(), 0);
	m_lineList.Render(renderer, parentTm);
	renderer.GetDevContext()->OMSetDepthStencilState(states.DepthDefault(), 0);
	return true;
}


void cPathRenderer::Clear()
{
	cNode::Clear();
	m_lineList.ClearLines();
}
