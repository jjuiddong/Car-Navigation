
#include "stdafx.h"
#include "qtile.h"

using namespace graphic;
using namespace gis2;


cQTile::cQTile()
	: m_loaded(false)
	, m_renderFlag(0x01 | 0x02)
	, m_modelLoadState(0)
	, m_texture(nullptr)
	, m_replaceTex(nullptr)
	, m_vtxBuff(nullptr)
	, m_hmap(nullptr)
	, m_replaceHmap(nullptr)
{
	ZeroMemory(m_loadFlag, sizeof(m_loadFlag));
}

cQTile::~cQTile()
{
	Clear();
}


bool cQTile::Create(graphic::cRenderer &renderer
	, const int level, const int x, const int y
	, const sRectf &rect
	, const char *textureFileName)
{
	m_level = level;
	m_loc = Vector2i(x, y);
	m_rect = rect;

	m_tuvs[0] = 0.f; // left-top u
	m_tuvs[1] = 0.f; // left-top v
	m_tuvs[2] = 1.f; // right-bottom u
	m_tuvs[3] = 1.f; // right-bottom v

	return true;
}


void cQTile::Render(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const XMMATRIX &tm //= graphic::XMIdentity
)
{
	Transform tfm;
	tfm.pos.x = (float)m_rect.left;
	tfm.pos.z = (float)m_rect.top;
	tfm.pos.y = 0.f;
	renderer.m_cbPerFrame.m_v->mWorld = XMMatrixTranspose(tfm.GetMatrixXM());
	renderer.m_cbPerFrame.Update(renderer);

	renderer.m_cbTessellation.m_v->size = Vector2((float)m_rect.Width(), (float)m_rect.Height());
	renderer.m_cbTessellation.m_v->level = (float)m_level;

	// bind texture
	if (m_texture)
	{
		renderer.m_cbTessellation.m_v->tuvs[0] = 0.f;
		renderer.m_cbTessellation.m_v->tuvs[1] = 0.f;
		renderer.m_cbTessellation.m_v->tuvs[2] = 1.f;
		renderer.m_cbTessellation.m_v->tuvs[3] = 1.f;
		m_texture->m_tex.Bind(renderer, 0);
	}
	else if (m_replaceTex)
	{
		renderer.m_cbTessellation.m_v->tuvs[0] = m_tuvs[0];
		renderer.m_cbTessellation.m_v->tuvs[1] = m_tuvs[1];
		renderer.m_cbTessellation.m_v->tuvs[2] = m_tuvs[2];
		renderer.m_cbTessellation.m_v->tuvs[3] = m_tuvs[3];
		m_replaceTex->m_tex.Bind(renderer, 0);
	}
	else
	{
		ID3D11ShaderResourceView *ns[1] = { NULL };
		renderer.GetDevContext()->PSSetShaderResources(0, 1, ns);
	}

	// bind heightmap
	if (m_hmap && m_hmap->IsLoaded())
	{
		renderer.m_cbTessellation.m_v->huvs[0] = 0.f;
		renderer.m_cbTessellation.m_v->huvs[1] = 0.f;
		renderer.m_cbTessellation.m_v->huvs[2] = 1.f;
		renderer.m_cbTessellation.m_v->huvs[3] = 1.f;
		m_hmap->m_texture->Bind(renderer, 8);
	}
	else if (m_replaceHmap)
	{
		renderer.m_cbTessellation.m_v->huvs[0] = m_huvs[0];
		renderer.m_cbTessellation.m_v->huvs[1] = m_huvs[1];
		renderer.m_cbTessellation.m_v->huvs[2] = m_huvs[2];
		renderer.m_cbTessellation.m_v->huvs[3] = m_huvs[3];
		m_replaceHmap->m_texture->Bind(renderer, 8);
	}
	else
	{
		ID3D11ShaderResourceView *ns[1] = { NULL };
		renderer.GetDevContext()->PSSetShaderResources(8, 1, ns);
	}

	renderer.m_cbTessellation.Update(renderer, 6);

	m_vtxBuff->Bind(renderer);
	renderer.GetDevContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	renderer.GetDevContext()->Draw(m_vtxBuff->GetVertexCount(), 0);
}


std::pair<bool, Vector3> cQTile::Pick(const Ray &ray)
{
	const float h0 = std::max(0.01f, GetHeight(Vector2(0, 0))); // left-top
	const float h1 = std::max(0.01f, GetHeight(Vector2(1, 0))); // right-top
	const float h2 = std::max(0.01f, GetHeight(Vector2(1, 1))); // right-bottom
	const float h3 = std::max(0.01f, GetHeight(Vector2(0, 1))); // left-bottom

	const Vector3 p0(m_rect.left, h0, m_rect.bottom);
	const Vector3 p1(m_rect.right, h1, m_rect.bottom);
	const Vector3 p2(m_rect.right, h2, m_rect.top);
	const Vector3 p3(m_rect.left, h3, m_rect.top);

	const Triangle tr1(p0, p1, p3);
	const Triangle tr2(p1, p2, p3);

	float t, u, v;
	if (tr1.Intersect(ray.orig, ray.dir, &t, &u, &v))
	{
		Vector3 pos1, pos2;
		lerp(pos1, tr1.a, tr1.b, u);
		lerp(pos2, tr1.a, tr1.c, v);
		return { true, {pos1 + pos2 - tr1.a } };
	}

	if (tr2.Intersect(ray.orig, ray.dir, &t, &u, &v))
	{
		Vector3 pos1, pos2;
		lerp(pos1, tr2.a, tr2.b, u);
		lerp(pos2, tr2.a, tr2.c, v);
		return { true,{ pos1 + pos2 - tr2.a } };
	}

	return { false, {} };
}


float cQTile::GetHeight(const Vector2 &uv) const
{
	if (m_hmap)
		return m_hmap->GetHeight(uv);
	else if (m_replaceHmap)
		return GetHeightByReplaceMap(m_replaceHmap, uv);
	return 0.f;
}


// calc height by argument hmap
float cQTile::GetHeightByReplaceMap(cHeightmap3 *hmap, const Vector2 &uv) const
{
	// original relation position by uv
	const Vector2 pos = Vector2(m_rect.left + m_rect.Width() * uv.x
		, m_rect.top - (m_rect.Height() * (uv.y - 1.f)));

	// parent tile rect
	const common::sRectf prect = gis2::GetNodeRect(
		hmap->m_level, hmap->m_x, hmap->m_y);

	// calc parent tile uv coordinate
	const float u = (pos.x - prect.left) / prect.Width();
	const float v = 1.f + (prect.top - pos.y) / prect.Height();
	return hmap->GetHeight(Vector2(u, v));
}


void cQTile::Clear()
{
	m_loaded = false;
	m_modelLoadState = 0;
}
