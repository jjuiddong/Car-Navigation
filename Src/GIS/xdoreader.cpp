
#include "stdafx.h"
#include "xdoreader.h"

using namespace graphic;


cXdoReader::cXdoReader()
	: m_loadState(READ_MODEL)
{
	ZeroMemory(m_key, sizeof(m_key));
}

cXdoReader::cXdoReader(graphic::cRenderer &renderer, const char *fileName)
	: m_loadState(READ_MODEL)
{
	Read(fileName, 0x02000003);
}

cXdoReader::~cXdoReader()
{
	Clear();
}


bool cXdoReader::Read(const char *fileName
	, const DWORD ver//=0x02000003
)
{
	using namespace std;

	Clear();

	ifstream ifs(fileName, ios::binary);
	if (!ifs.is_open())
		return false;

	m_fileName = fileName;

	ReadV2(ifs);

	return true;
}


// ver 3.0.0.1
bool cXdoReader::ReadV1(std::ifstream &ifs)
{
	return true;
}


// ver 3.0.0.2
bool cXdoReader::ReadV2(std::ifstream &ifs)
{
	ifs.read((char*)&m_type, sizeof(m_type));
	ifs.read((char*)&m_objectId, sizeof(m_objectId));

	BYTE keyLen = 0;
	ifs.read((char*)&keyLen, sizeof(keyLen));
	ifs.read(m_key, min(sizeof(m_key) - 1, (uint)keyLen));

	ifs.read((char*)&m_objBox, sizeof(m_objBox));
	ifs.read((char*)&m_altitude, sizeof(m_altitude));
	ifs.read((char*)&m_faceNum, sizeof(m_faceNum));

	for (int i = 0; i < m_faceNum; ++i)
	{
		sXdo xdo;
		if (!Read3DMeshDataBlock(ifs, xdo))
			break;
		m_xdos.push_back(xdo);
	}

	return true;
}


bool cXdoReader::Read3DMeshDataBlock(std::ifstream &ifs, OUT sXdo &out)
{
	ZeroMemory(&out, sizeof(out));

	ifs.read((char*)&out.vertexCount, sizeof(out.vertexCount));
	if (out.vertexCount > 0)
	{
		out.vertices = new sVertexNormTex[out.vertexCount];
		ifs.read((char*)out.vertices, out.vertexCount * sizeof(sVertexNormTex));
	}

	ifs.read((char*)&out.indexCount, sizeof(out.indexCount));
	if (out.indexCount > 0)
	{
		out.indices = new u_short[out.indexCount];
		ifs.read((char*)out.indices, out.indexCount * sizeof(u_short));
	}

	ifs.read((char*)&out.color, sizeof(out.color));
	ifs.read((char*)&out.imageLevel, sizeof(out.imageLevel));

	BYTE imageNameLen = 0;
	ifs.read((char*)&imageNameLen, sizeof(imageNameLen));
	ifs.read(out.imageName, min(sizeof(out.imageName) - 1, (uint)imageNameLen));

	return true;
}


// http://www.vw-lab.com/53
Vector3 rotate3d(float vx, float vy, float vz, double lon, double lat) 
{
	float x, y, z;
	double p = (lon) / 180 * MATH_PI;
	double t = (90 - lat) / 180 * MATH_PI;

	//원래 회전공식대로 하니까 90도 회전된 결과가 나와 z축을 중심으로 다시 -90도 회전을 했다.
	z = (float)(cos(t)*cos(p)*vx - cos(t)* sin(p) * vy - sin(t)*vz);
	x = 1 * (float)(sin(p)*vx + cos(p)*vy);
	y = (float)(sin(t)*cos(p)*vx - sin(t)*sin(p)*vy + cos(t)*vz);

	return {x, y, z};
}


// 메쉬를 그래픽 메모리에 로딩한다.
bool cXdoReader::LoadMesh(graphic::cRenderer &renderer)
{
	sRawMeshGroup2 *meshGrp = new sRawMeshGroup2;
	StrId modelFileName = m_fileName.c_str();
	meshGrp->name = modelFileName;
	int id = 0;
	for (auto &xdo : m_xdos)
	{
		if ((xdo.vertexCount <= 0) || (xdo.indexCount < 0))
			continue;

		meshGrp->meshes.push_back({});

		sRawMesh2 &mesh = meshGrp->meshes.back();
		mesh.renderFlags = eRenderFlag::VISIBLE | eRenderFlag::NOALPHABLEND;
		mesh.vertices.reserve(xdo.vertexCount);
		mesh.indices.reserve(xdo.indexCount);
		//mesh.normals.reserve(xdo.vertexCount);
		mesh.tex.reserve(xdo.vertexCount);
		mesh.mtrl.texture = StrPath(m_fileName.GetFileNameExceptExt2().c_str()) + ".jpg";
		mesh.mtrl.ambient = Vector4(0, 0, 0, 1);
		mesh.mtrl.diffuse = Vector4(1, 1, 1, 1);
		mesh.mtrl.specular = Vector4(0, 0, 0, 1);
		mesh.mtrl.emissive = Vector4(0, 0, 0, 1);
		mesh.mtrl.power = 1.f;

		//cResourceManager::Get()->LoadTexture(renderer, mesh.mtrl.texture);

		for (uint i = 0; i < xdo.vertexCount; ++i)
		{
			const Vector3 p = xdo.vertices[i].p;
			const Vector3 np = rotate3d(p.x, p.y, p.z, m_lonLat.x, m_lonLat.y);
			mesh.vertices.push_back(np);
			//mesh.vertices.push_back(Vector3(xdo.vertices[i].p.x, xdo.vertices[i].p.z, xdo.vertices[i].p.y));
		}

		//for (u_int i = 0; i < xdo.vertexCount; ++i)
		//	mesh.vertices.push_back(Vector3(xdo.vertices[i].p.x, xdo.vertices[i].p.z, xdo.vertices[i].p.y));
			//mesh.vertices.push_back(Vector3(xdo.vertices[i].p.x, xdo.vertices[i].p.y, xdo.vertices[i].p.z));
			//mesh.vertices.push_back(Vector3(-xdo.vertices[i].p.x, -xdo.vertices[i].p.y, xdo.vertices[i].p.z));


			//mesh.vertices.push_back(Vector3(xdo.vertices[i].p.x, -xdo.vertices[i].p.y, xdo.vertices[i].p.z));
			//mesh.vertices.push_back(Vector3(xdo.vertices[i].p.z, xdo.vertices[i].p.x, xdo.vertices[i].p.y));

			//mesh.vertices.push_back(Vector3(xdo.vertices[i].p.x, xdo.vertices[i].p.y, xdo.vertices[i].p.z));
			//mesh.vertices.push_back(xdo.vertices[i].p);
		//for (u_int i = 0; i < xdo.indexCount; ++i)
		//	mesh.indices.push_back(xdo.indices[i]);

		for (uint i = 0; i < xdo.indexCount; i+=3)
		{
			mesh.indices.push_back(xdo.indices[i]);
			mesh.indices.push_back(xdo.indices[i+1]);
			mesh.indices.push_back(xdo.indices[i+2]);
		}

		//for (u_int i = 0; i < xdo.vertexCount; ++i)
		//	mesh.normals.push_back(xdo.vertices[i].n);
		for (uint i = 0; i < xdo.vertexCount; ++i)
			mesh.tex.push_back(Vector3(xdo.vertices[i].u, xdo.vertices[i].v, 0));

		meshGrp->nodes.push_back({});
		sRawNode &node = meshGrp->nodes.back();
		node.name.Format("node%d", id++);
		node.meshes.push_back(0);
		node.localTm.SetIdentity();
	}

	graphic::cResourceManager::Get()->InsertRawMesh(meshGrp->name.c_str(), meshGrp);

	cAssimpModel *assimpModel = new cAssimpModel();
	assimpModel->Create(renderer, modelFileName.c_str());
	graphic::cResourceManager::Get()->InsertAssimpModel(meshGrp->name.c_str(), assimpModel);
	
	return true;
}


void cXdoReader::CalcAltitude(const Vector2d &lonLat)
{
	const double objX = (m_objBox._min.x + m_objBox._max.x) / 2.f;
	const double objY = (m_objBox._min.y + m_objBox._max.y) / 2.f;
	const double objZ = (m_objBox._min.z + m_objBox._max.z) / 2.f;
	const Vector3 p = rotate3d((float)objX, (float)objY, (float)objZ, lonLat.x, lonLat.y);
	m_altitude2 = (float)((double)p.y - 6378137.f);
}


Quaternion cXdoReader::GetRotation(const common::Vector2 &offsetLonLat)
{
	Vector2d lonLat = m_lonLat;
	lonLat.x += (double)offsetLonLat.x;
	lonLat.y += (double)offsetLonLat.y;

	Vector3 longitude((float)cos(ANGLE2RAD(lonLat.x)), 0, (float)sin(ANGLE2RAD(lonLat.x)));
	longitude.Normalize();
	Quaternion q1;
	q1.SetRotationY(ANGLE2RAD(90));
	longitude *= q1.GetMatrix();
	Vector3 normal = longitude * (float)cos(ANGLE2RAD(lonLat.y)) 
		+ Vector3(0, 1, 0) * (float)sin(ANGLE2RAD(lonLat.y));
	normal.Normalize();

	Quaternion q2;
	q2.SetRotationArc(normal, Vector3(0, 1, 0));
	return q2;
}


void cXdoReader::Clear()
{
	for (auto &xdo : m_xdos)
	{
		delete[] xdo.vertices;
		delete[] xdo.indices;
	}
	m_xdos.clear();
}


StrPath cXdoReader::GetFileName(const StrPath &directoryName, const int level, const int xLoc, const int yLoc
	, const char *dataFileName)
{
	StrPath path;
	path.Format("%s\\%d\\%04d\\%s", directoryName.c_str(), level, yLoc, dataFileName);
	return path;
}
