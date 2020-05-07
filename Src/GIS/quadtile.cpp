
#include "stdafx.h"
#include "quadtile.h"


using namespace graphic;

cQuadTile::cQuadTile()
	: m_texture(NULL)
	, m_loaded(false)
	, m_modelLoadState(0)
	, m_replaceTex(NULL)
	, m_vtxBuff(NULL)
	, m_replaceTexLv(-1)
	, m_replaceHeightLv(-1)
	, m_hmap(NULL)
	, m_deepCpyHmap(NULL)
	, m_replaceHmap(NULL)
	, m_renderFlag(0x01 | 0x02)
{
	ZeroMemory(m_poi, sizeof(m_poi));
	ZeroMemory(m_loadFlag, sizeof(m_loadFlag));
}

cQuadTile::~cQuadTile()
{
	Clear();
}


bool cQuadTile::Create(graphic::cRenderer &renderer
	, const int level, const int xLoc, const int yLoc
	, const sRectf &rect
	, const char *textureFileName)
{
	m_level = level;
	m_loc = Vector2i(xLoc, yLoc);
	m_rect = rect;
	
	m_tuvs[0] = 0.f; // left-top u
	m_tuvs[1] = 0.f; // left-top v
	m_tuvs[2] = 1.f; // right-bottom u
	m_tuvs[3] = 1.f; // right-bottom v

	return true;
}


// adjLevel: eDirection index, adjacent quad level
void cQuadTile::Render(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const int adjLevel[4]
	, const XMMATRIX &tm //= graphic::XMIdentity
)
{
	RenderGeometry(renderer, deltaSeconds, adjLevel, tm);
}


void cQuadTile::RenderGeometry(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const int adjLevel[4]
	, const XMMATRIX &tm //= graphic::XMIdentity
)
{
	Transform tfm;
	tfm.pos.x = (float)m_rect.left;
	tfm.pos.z = (float)m_rect.top;
	tfm.pos.y = 0.f;
	renderer.m_cbPerFrame.m_v->mWorld = XMMatrixTranspose(tfm.GetMatrixXM());
	renderer.m_cbPerFrame.Update(renderer);

	//  Z axis
	// /|\
	//  |
	//  |
	//  |
	// -----------> X axis
	//
	//	 Clock Wise Tessellation Quad
	//        North
	//  * ------1--------*
	//  |                |
	//  |                |
	//  0 West           2 East
	//  |                |
	//  |                |
	//  * ------3--------*
	//         South

	renderer.m_cbTessellation.m_v->size = Vector2((float)m_rect.Width(), (float)m_rect.Height());
	renderer.m_cbTessellation.m_v->level = (float)m_level;
	renderer.m_cbTessellation.m_v->edgeLevel[0] = (float)adjLevel[eDirection::WEST];
	renderer.m_cbTessellation.m_v->edgeLevel[1] = (float)adjLevel[eDirection::NORTH];
	renderer.m_cbTessellation.m_v->edgeLevel[2] = (float)adjLevel[eDirection::EAST];
	renderer.m_cbTessellation.m_v->edgeLevel[3] = (float)adjLevel[eDirection::SOUTH];

	// Texture Bind
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

	// Heightmap Bind
	if (m_hmap && (m_hmap->m_waitState == 1))
	{
		//m_hmap->UpdateTexture(renderer);
		m_hmap->m_waitState = 0;
		m_hmap->m_isTextureUpdate = true;
		//ZeroMemory(&m_hmap->m_desc, sizeof(m_hmap->m_desc));
		//HRESULT hr = renderer.GetDevContext()->Map(m_hmap->m_texture->m_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &m_hmap->m_desc);
		//m_hmap->m_waitState = 2;
	}
	//if (m_hmap && (m_hmap->m_waitState == 3))
	//{
	//	renderer.GetDevContext()->Unmap(m_hmap->m_texture->m_texture, 0);
	//	m_hmap->m_waitState = 0;
	//}

	if (m_hmap && m_hmap->IsLoaded())
	{
		renderer.m_cbTessellation.m_v->huvs[0] = 0.f;
		renderer.m_cbTessellation.m_v->huvs[1] = 0.f;
		renderer.m_cbTessellation.m_v->huvs[2] = 1.f;
		renderer.m_cbTessellation.m_v->huvs[3] = 1.f;
		m_hmap->m_texture->Bind(renderer, 8);
	}
	else if (m_deepCpyHmap && m_deepCpyHmap->IsLoaded())
	{
		renderer.m_cbTessellation.m_v->huvs[0] = 0.f;
		renderer.m_cbTessellation.m_v->huvs[1] = 0.f;
		renderer.m_cbTessellation.m_v->huvs[2] = 1.f;
		renderer.m_cbTessellation.m_v->huvs[3] = 1.f;
		m_deepCpyHmap->m_texture->Bind(renderer, 8);
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


void cQuadTile::RenderPoi(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const bool isShowPoi1
	, const bool isShowPoi2
	, const XMMATRIX &tm //= graphic::XMIdentity
)
{
	if (m_renderFlag & 0x02)
	{
		// 카메라와 거리가 멀면 출력하지 않는다.
		const Vector3 eyePos = GetMainCamera().GetEyePos();
		const Vector2 center = m_rect.Center();
		const float dist = eyePos.Distance(Vector3(center.x, 0.f, center.y));
		if (dist > (eyePos.y * 5))
			return;
	}

	if (isShowPoi1 && m_poi[1])
	{
		int n = 0;
		for (auto &poi : m_poi[1]->m_pois)
		{
			if (poi.relPos.IsEmpty())
			{
				const Vector3 relPos = gis::GetRelationPos(gis::WGS842Pos(poi.lonLat));
				poi.relPos = relPos + Vector3(0, poi.altitude * 0.00001f * 2500.f, 0);
			}

			Transform txtTfm;
			txtTfm.pos = poi.relPos;
			renderer.m_textMgr.AddTextRender(renderer, (__int64)m_poi[1] + n++, poi.text.c_str()
				, cColor::WHITE, cColor::BLACK
				, graphic::BILLBOARD_TYPE::DYN_SCALE
				, txtTfm, true, 16, 1, 0.0001f);
		}
	}

	if (isShowPoi2 && m_poi[0])
	{
		int n = 0;
		for (auto &poi : m_poi[0]->m_pois)
		{
			if (poi.relPos.IsEmpty())
			{
				const Vector3 relPos = gis::GetRelationPos(gis::WGS842Pos(poi.lonLat));
				poi.relPos = relPos + Vector3(0, poi.altitude * 0.00001f * 2500.f, 0);
			}

			Transform txtTfm;
			txtTfm.pos = poi.relPos;
			txtTfm.scale = Vector3(1, 1, 1) * 0.7f;
			renderer.m_textMgr.AddTextRender(renderer, (__int64)m_poi[0] + n++, poi.text.c_str()
				, cColor::GRAY2, cColor::BLACK
				, graphic::BILLBOARD_TYPE::DYN_SCALE
				, txtTfm, true, 16, 1, 0.0001f);
		}
	}
}


void cQuadTile::RenderFacility(graphic::cRenderer &renderer
	, const float deltaSeconds
	, const XMMATRIX &tm //= graphic::XMIdentity
)
{
	for (u_int i = 0; i < m_facilities.size(); ++i)
	{
		cXdoReader *xdo = m_facilities[i];
		if (!xdo)
			continue;
		if (cXdoReader::FINISH != xdo->m_loadState)
			continue;
		m_mods[i].Render(renderer);		
	}

	//if (m_facility && m_facilityTex)
	//{
	//	m_mod.Render(renderer);
	//}
}


// this와 tile이 겹치는 부분의 높이값을 서로 똑같게 한다.
// direction: this를 기준으로 tile의 위치
bool cQuadTile::Smooth(cQuadTile &tile, const eDirection::Enum direction)
{
#define MACRO_SWITCH32(val, expr) \
switch (val) \
{ \
case 32: expr; \
case 31: expr; \
case 30: expr; \
case 29: expr; \
case 28: expr; \
case 27: expr; \
case 26: expr; \
case 25: expr; \
case 24: expr; \
case 23: expr; \
case 22: expr; \
case 21: expr; \
case 20: expr; \
case 19: expr; \
case 18: expr; \
case 17: expr; \
case 16: expr; \
case 15: expr; \
case 14: expr; \
case 13: expr; \
case 12: expr; \
case 11: expr; \
case 10: expr; \
case 9: expr; \
case 8: expr; \
case 7: expr; \
case 6: expr; \
case 5: expr; \
case 4: expr; \
case 3: expr; \
case 2: expr; \
case 1: expr; \
	break; \
}

	RETV((direction != eDirection::WEST) && (direction != eDirection::SOUTH), false);

	cHeightmap2 *srcMap = (tile.m_hmap)? tile.m_hmap : tile.m_replaceHmap;
	cHeightmap2 *dstMap = (m_hmap) ? m_hmap : m_replaceHmap;
	RETV(!srcMap || !dstMap, false);
	RETV(!srcMap->m_data || !dstMap->m_data, false);

	// rect 와 3d 상의 rect 는 z축이 뒤바뀐 상태다. 
	// 그래서, bottom 이 top 이되고, top 이 bottom 이 된다.
	// uv 계산시 발생하는 음수는 항상 양수로 계산하면 문제 없다.
	const Vector2 lt = Vector2((float)tile.m_rect.left, (float)tile.m_rect.bottom); // left-top
	const Vector2 rb = Vector2((float)tile.m_rect.right, (float)tile.m_rect.top); // right-bottom
	const Vector2 clt = Vector2((float)m_rect.left, (float)m_rect.bottom); // current rect left-top
	const Vector2 crb = Vector2((float)m_rect.right, (float)m_rect.top); // current rect right-bottom

	if (eDirection::WEST == direction)
	{
		const float dstV1 = max(0.f, clt.y - lt.y) / m_rect.Height();
		const float dstV2 = 1.f - (max(0.f, rb.y - crb.y) / m_rect.Height());
		const float srcV1 = max(0.f, lt.y - clt.y) / tile.m_rect.Height();
		const float srcV2 = 1.f - (max(0.f, crb.y - rb.y) / tile.m_rect.Height());

		int dstY = (int)(dstV1 * (dstMap->m_height));
		int dstEndY = (int)(dstV2 * (dstMap->m_height));
		int srcY = (int)(srcV1 * (srcMap->m_height));
		int srcEndY = (int)(srcV2 * (srcMap->m_height));
		const int dstVtxCnt = max(1, (int)pow(2, (6 - max(0, m_level - tile.m_level)))); // 1 ~ 64
		const int srcVtxCnt = max(1, (int)pow(2, (6 - max(0, tile.m_level - m_level)))); // 1 ~ 64
		const int incDst = dstMap->m_height / dstVtxCnt;
		const int incSrc = srcMap->m_height / srcVtxCnt;

		while (srcY < srcEndY)
		{
			const float val = srcMap->m_data[srcY * srcMap->m_width + (srcMap->m_width - 1)];
			//dstMap->m_data[dstY * dstMap->m_width] = val;
			//dstY += incDst;
			MACRO_SWITCH32(incDst, dstMap->m_data[dstY++ * dstMap->m_width] = val);
			srcY += incSrc;
		}
		dstMap->m_data[(dstEndY-1) * dstMap->m_width] = 
			srcMap->m_data[min(srcMap->m_height-1, srcEndY) * srcMap->m_width + (srcMap->m_width - 1)];
	}
	else if (eDirection::SOUTH == direction)
	{
		const float dstU1 = max(0.f, lt.x - clt.x) / m_rect.Width();
		const float dstU2 = 1.f - (max(0.f, crb.x - rb.x) / m_rect.Width());
		const float srcU1 = max(0.f, clt.x - lt.x) / tile.m_rect.Width();
		const float srcU2 = 1.f - (max(0.f, rb.x - crb.x) / tile.m_rect.Width());

		int dstX = (int)(dstU1 * (dstMap->m_width));
		int dstEndX = (int)(dstU2 * (dstMap->m_width));
		int srcX = (int)(srcU1 * (srcMap->m_width));
		int srcEndX = (int)(srcU2 * (srcMap->m_width));
		const int dstVtxCnt = max(1, (int)pow(2, (6 - max(0, m_level - tile.m_level)))); // 1 ~ 64
		const int srcVtxCnt = max(1, (int)pow(2, (6 - max(0, tile.m_level - m_level)))); // 1 ~ 64
		const int incDst = dstMap->m_width / dstVtxCnt;
		const int incSrc = srcMap->m_width / srcVtxCnt;
		const int dstOffset = (dstMap->m_height - 1) * m_hmap->m_width;

		while (srcX < srcEndX)
		{
			const float val = srcMap->m_data[srcX];
			//dstMap->m_data[dstOffset + dstX] = val;
			//dstX += incDst;
			MACRO_SWITCH32(incDst, dstMap->m_data[dstOffset + dstX++] = val);
			srcX += incSrc;
		}
		dstMap->m_data[dstOffset + (dstEndX - 1)] = srcMap->m_data[min(srcMap->m_width-1, srcEndX)];
	}

	return true;
}


// Smooth처리 후, West South 꼭지점의 겹치는 부분을 처리한다.
// West South 꼭지점 3 픽셀 값의 평균을 저장한다.
void cQuadTile::SmoothPostProcess(cQuadTile &southWestTile)
{
	cHeightmap2 *srcMap = (southWestTile.m_hmap) ? southWestTile.m_hmap : southWestTile.m_replaceHmap;
	RET(!srcMap);

	const float h1 = srcMap->m_data[srcMap->m_width-1];
	//m_hmap.m_data[(m_hmap.m_height - 1) * m_hmap.m_width] = h1;

	//const float h2 = m_hmap.m_data[(m_hmap.m_height - 2) * m_hmap.m_width + 1];
	//const float h3 = m_hmap.m_data[(m_hmap.m_height - 1) * m_hmap.m_width + 1];
	//m_hmap.m_data[(m_hmap.m_height - 1) * m_hmap.m_width] = (h1 + h2 + h3) / 3.f;
	//m_hmap.m_data[(m_hmap.m_height - 1) * m_hmap.m_width] = (h1 + h3) / 2.f;
}


// 높이맵을 텍스쳐에 업데이트 한다.
void cQuadTile::UpdateTexture(graphic::cRenderer &renderer)
{
	D3D11_MAPPED_SUBRESOURCE desc;
	if (BYTE *dst = (BYTE*)m_hmap->m_texture->Lock(renderer, desc))
	{
		float *src = m_hmap->m_data;
		const int srcPitch = m_hmap->m_width;
		for (int i = 0; i < m_hmap->m_height; ++i)
		{
			memcpy(dst, src, srcPitch * sizeof(float));
			dst += desc.RowPitch;
			src += srcPitch;
		}
		m_hmap->m_texture->Unlock(renderer);
	}
}


float cQuadTile::GetHeight(const Vector2 &uv) const
{
	if (m_hmap)
		return m_hmap->GetHeight(uv) - 0.1f;
	else if (m_deepCpyHmap)
		return GetHeightByReplaceMap(m_deepCpyHmap, uv);
	else if (m_replaceHmap)
		return GetHeightByReplaceMap(m_replaceHmap, uv);
	return 0.f;
}


// calc height by argument hmap
float cQuadTile::GetHeightByReplaceMap(cHeightmap2 *hmap, const Vector2 &uv) const
{
	// original relation position by uv
	const Vector2 pos = Vector2(m_rect.left + m_rect.Width() * uv.x
		, m_rect.top - (m_rect.Height() * (uv.y - 1.f)));

	// parent tile rect
	const common::sRectf prect = cQuadTree<cQuadTile>::GetNodeRect(
		hmap->m_level, hmap->m_xLoc
		, hmap->m_yLoc);

	// calc parent tile uv coordinate
	const float u = (pos.x - prect.left) / prect.Width();
	const float v = 1.f + (prect.top - pos.y) / prect.Height();
	return hmap->GetHeight(Vector2(u, v)) - 0.1f;
}


std::pair<bool, Vector3> cQuadTile::Pick(const Ray &ray)
{
	const float h0 = GetHeight(Vector2(0, 0)) * 2500.f; // left-top
	const float h1 = GetHeight(Vector2(1, 0)) * 2500.f; // right-top
	const float h2 = GetHeight(Vector2(1, 1)) * 2500.f; // right-bottom
	const float h3 = GetHeight(Vector2(0, 1)) * 2500.f; // left-bottom
	
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


void cQuadTile::Clear()
{
	m_loaded = false;
	m_modelLoadState = 0;
}
