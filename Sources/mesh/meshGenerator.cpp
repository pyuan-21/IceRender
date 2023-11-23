#define _USE_MATH_DEFINES
#include <math.h>
#include "meshGenerator.hpp"
#include <vector>
#include "glm/gtx/quaternion.hpp"
#include "../helpers/utility.hpp"

using namespace std;
using namespace IceRender;

shared_ptr<Mesh> MeshGenerator::GenSphere(const size_t& _hNum, const size_t& _vNum, const float& _radius)
{
	// construct sphere at origin.
	vector<glm::vec3> pos;
	vector<glm::vec3> normals;
	vector<glm::uvec3> indices;

	// using right hand coordinate system. up is y, left is x, toward to user is z
	float dv = static_cast<float>(M_PI / _vNum); // delta angle in vertical direction
	float dh = static_cast<float>(2 * M_PI / _hNum); // delta angle in horizontal direction
	for (size_t i = 0; i <= _vNum; i++)
	{
		float vRadians = dv * i;
		float y = cos(vRadians);
		float xz = sin(vRadians);
		for (size_t j = 0; j <= _hNum; j++)
		{
			float hRadians = dh * j;
			float x = xz * cos(hRadians);
			float z = xz * sin(hRadians);
			glm::vec3 n(x, y, z);
			normals.push_back(n);
			glm::vec3 p = n * _radius;
			pos.push_back(p);

			if (j == _hNum)
				continue;

			// Below, be careful here, don't forgert put parentheses to _i, and _j, otherewise it will become i+1%(_vNum+1) then will get wrong result
#define index(_i, _j) ((_i)%(_vNum+1)*(_hNum+1)+(_j)) 
			// k3 -- k1
			// k4 -- k2
			size_t k1 = index(i, j); // let K1 is the index of pos when this point at (i,j)
			size_t k2 = index(i, j + 1); // similar as above
			size_t k3 = index(i + 1, j);
			size_t k4 = index(i + 1, j + 1);
#undef index
			// two triangles: k1-k4-k3, k1-k2-k4
			indices.push_back(glm::uvec3(k1, k4, k3));
			indices.push_back(glm::uvec3(k1, k2, k4));
		}
	}

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->SetPositions(pos);
	mesh->SetNormals(normals);
	mesh->SetIndices(indices);
	return mesh;
}


shared_ptr<Mesh> MeshGenerator::GenPlane(const float& _width, const float& _height, const glm::vec3& _center, const glm::vec3& _normal, bool isBackGround)
{
	// just create two triangles to construct a plane
	// first create a plane with normal pointing to positive z-axis

	int depth = isBackGround ? 1 : 0;
	glm::vec3 tl = _center + glm::vec3(-_width * 0.5, _height * 0.5, depth); // top left
	glm::vec3 tr = _center + glm::vec3(_width * 0.5, _height * 0.5, depth); // top right
	glm::vec3 bl = _center + glm::vec3(-_width * 0.5, -_height * 0.5, depth); // bottom left
	glm::vec3 br = _center + glm::vec3(_width * 0.5, -_height * 0.5, depth); // bottom right
	glm::vec3 n = glm::normalize(_normal);
	if (n != Utility::backV3 && n != Utility::frontV3) {
		float angle = Utility::GetAngleInRadians(Utility::backV3, n);
		glm::quat qua = Utility::GetQuaternion(Utility::backV3, n, angle);
		tl = qua * tl;
		tr = qua * tr;
		bl = qua * bl;
		br = qua * br;
	}

	vector<glm::vec3> pos{ tl,tr,bl,br };
	vector<glm::vec3> normals{ n,n,n,n };

	// indices should be initialized by its normal, to make it counter clockwise with its normal
	vector<glm::uvec3> indices;
	glm::vec3 tl_tr = tr - tl;
	glm::vec3 tl_bl = bl - tl;
	glm::vec3 crs = glm::cross(tl_bl, tl_tr); // rotated axis which rotates 'tl_bl' to 'tl_tr'
	float cosangle = glm::dot(crs, n); // to check which direction is front face(counter clockwise order)
	if (cosangle >= 0)
		indices = { glm::uvec3(0,2,1),glm::uvec3(1,2,3) };
	else
		indices = { glm::uvec3(0,1,2),glm::uvec3(1,3,2) };

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->SetPositions(pos);
	mesh->SetIndices(indices);
	mesh->SetNormals(normals);
	return mesh;
}

shared_ptr<Mesh> MeshGenerator::GenCube(const float& _length)
{
	// _length here is the length of edge
	// each point has tree faces, then it should have three normals
	// to do that, duplicate each point of cube in three times
	float hl = _length / 2; // half length
	vector<glm::vec3> positions;
	vector<glm::vec3> normals;
	vector<glm::uvec3> indices;

	// positive x
	positions.push_back(glm::vec3(hl, hl, hl));		normals.push_back(Utility::rightV3); // v1
	positions.push_back(glm::vec3(hl, -hl, hl));	normals.push_back(Utility::rightV3); // v2
	positions.push_back(glm::vec3(hl, -hl, -hl));	normals.push_back(Utility::rightV3); // v3
	positions.push_back(glm::vec3(hl, hl, -hl));	normals.push_back(Utility::rightV3); // v4
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 4, positions.size() - 3)); // v4-v1-v2
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 3, positions.size() - 2)); // v4-v2-v3
	// negative x
	positions.push_back(glm::vec3(-hl, hl, -hl));	normals.push_back(Utility::leftV3); // v1
	positions.push_back(glm::vec3(-hl, -hl, -hl));	normals.push_back(Utility::leftV3); // v2
	positions.push_back(glm::vec3(-hl, -hl, hl));	normals.push_back(Utility::leftV3); // v3
	positions.push_back(glm::vec3(-hl, hl, hl));	normals.push_back(Utility::leftV3); // v4
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 4, positions.size() - 3)); // v4-v1-v2
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 3, positions.size() - 2)); // v4-v2-v3
	// positive y
	positions.push_back(glm::vec3(-hl, hl, -hl));	normals.push_back(Utility::upV3); // v1
	positions.push_back(glm::vec3(-hl, hl, hl));	normals.push_back(Utility::upV3); // v2
	positions.push_back(glm::vec3(hl, hl, hl));		normals.push_back(Utility::upV3); // v3
	positions.push_back(glm::vec3(hl, hl, -hl));	normals.push_back(Utility::upV3); // v4
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 4, positions.size() - 3)); // v4-v1-v2
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 3, positions.size() - 2)); // v4-v2-v3
	// negative y
	positions.push_back(glm::vec3(-hl, -hl, hl));	normals.push_back(Utility::downV3); // v1
	positions.push_back(glm::vec3(-hl, -hl, -hl));	normals.push_back(Utility::downV3); // v2
	positions.push_back(glm::vec3(hl, -hl, -hl));	normals.push_back(Utility::downV3); // v3
	positions.push_back(glm::vec3(hl, -hl, hl));	normals.push_back(Utility::downV3); // v4
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 4, positions.size() - 3)); // v4-v1-v2
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 3, positions.size() - 2)); // v4-v2-v3
	// positive z
	positions.push_back(glm::vec3(-hl, hl, hl));	normals.push_back(Utility::backV3); // v1
	positions.push_back(glm::vec3(-hl, -hl, hl));	normals.push_back(Utility::backV3); // v2
	positions.push_back(glm::vec3(hl, -hl, hl));	normals.push_back(Utility::backV3); // v3
	positions.push_back(glm::vec3(hl, hl, hl));		normals.push_back(Utility::backV3); // v4
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 4, positions.size() - 3)); // v4-v1-v2
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 3, positions.size() - 2)); // v4-v2-v3 
	//// negative z
	positions.push_back(glm::vec3(hl, hl, -hl));	normals.push_back(Utility::frontV3); // v1
	positions.push_back(glm::vec3(hl, -hl, -hl));	normals.push_back(Utility::frontV3); // v2
	positions.push_back(glm::vec3(-hl, -hl, -hl));	normals.push_back(Utility::frontV3); // v3
	positions.push_back(glm::vec3(-hl, hl, -hl));	normals.push_back(Utility::frontV3); // v4
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 4, positions.size() - 3)); // v4-v1-v2
	indices.push_back(glm::uvec3(positions.size() - 1, positions.size() - 3, positions.size() - 2)); // v4-v2-v3

	shared_ptr<Mesh> mesh = make_shared<Mesh>();
	mesh->SetPositions(positions);
	mesh->SetNormals(normals);
	mesh->SetIndices(indices);
	return mesh;
}

vector<glm::vec2> MeshGenerator::GenSphereUV(const size_t& _hNum, const size_t& _vNum)
{
	// same method of GenSphere()
	vector<glm::vec2> uv;
	for (size_t i = 0; i <= _vNum; i++)
		for (size_t j = 0; j <= _hNum; j++)
			uv.push_back(glm::vec2(1 - static_cast<float>(j) / _hNum, 1 - static_cast<float>(i) / _vNum));
	return uv;
}

std::vector<glm::vec2> MeshGenerator::GenPlaneUV()
{
	// because Plane has only four points: tl,tr,bl,br
	// therefore, their UV are very obvious.
	return std::vector<glm::vec2>{glm::vec2(0, 1), glm::vec2(1, 1), glm::vec2(0, 0), glm::vec2(1, 0)};
}

shared_ptr<Mesh> MeshGenerator::GenMeshFromOFF(const std::string _fileName)
{
	// Load an OFF file. See https://en.wikipedia.org/wiki/OFF_(file_format)
	std::string filePath = "Resources/Models/OFF/" + _fileName;
	std::ifstream s(filePath);
	if (!s.is_open())
	{
		Print("[Error] failed to open: " + filePath);
		return nullptr;
	}

	std::string line;
	int vNum = 0, fNum = 0; // vertices, faces number (Igonre edges here)
	bool usingQuad = false;
	int flag = 0;
	vector<glm::vec3> pos;
	vector<glm::vec3> normals;
	vector<glm::uvec3> indices;
	// not support read color at each vertex
	while (std::getline(s, line))
	{
		if (line.empty())
			continue;
		if (line[0] == '#')
			continue; // skip comment
		if (line.find("OFF") != std::string::npos)
			continue; // OFF letters are optional
		std::istringstream iss(line);
		if (flag == 0)
		{
			if (iss >> vNum >> fNum)
			{
				normals.resize(vNum, Utility::zeroV3);
				flag++;
			}
			else
			{
				Print("[Error] can not read the number of vertice, faces.");
				break;
			}
		}
		else if (flag == 1)
		{
			// reading position
			float x, y, z;
			iss >> x >> y >> z;
			pos.push_back(glm::vec3(x, y, z));
			if (pos.size() == vNum)
				flag++; // enter reading faces
		}
		else if (flag == 2)
		{
			// reading faces
			int pNum; // how many vertex in such face(triangle)
			iss >> pNum;
			if (pNum == 3)
			{
				int v1, v2, v3;
				iss >> v1 >> v2 >> v3;
				indices.push_back(glm::uvec3(v1, v2, v3));
				// normal is computed by using counter clockwise direction
				glm::vec3 e12 = pos[v2] - pos[v1]; // from vertex 1 to vertex 2
				glm::vec3 e13 = pos[v3] - pos[v1]; // from 1 to 3
				glm::vec3 n = glm::normalize(glm::cross(e12, e13));
				normals[v1] += n;
				normals[v2] += n;
				normals[v3] += n;
			}
			else if (pNum == 4)
			{
				usingQuad = true;
				int v1, v2, v3, v4;
				iss >> v1 >> v2 >> v3 >> v4;
				// face v1-v2-v3
				indices.push_back(glm::uvec3(v1, v2, v3));
				// normal is computed by using counter clockwise direction
				glm::vec3 e12 = pos[v2] - pos[v1]; // from vertex 1 to vertex 2
				glm::vec3 e13 = pos[v3] - pos[v1]; // from 1 to 3
				glm::vec3 n = glm::normalize(glm::cross(e12, e13));
				normals[v1] += n;
				normals[v2] += n;
				normals[v3] += n;
				// face v1,v3,v4
				indices.push_back(glm::uvec3(v1, v3, v4));
				// normal is computed by using counter clockwise direction
				glm::vec3 e14 = pos[v4] - pos[v1]; // from 1 to 4
				n = glm::normalize(glm::cross(e13, e14));
				normals[v1] += n;
				normals[v3] += n;
				normals[v4] += n;
			}
			else
			{
				Print("[Error] For now only 3 or 4 vertices in a face can be handled.");
				break;
			}
		}
	}

	// normalized normals
	for (size_t i = 0; i < normals.size(); i++)
		normals[i] = glm::normalize(normals[i]);

	if (usingQuad)
		fNum *= 2; // when using quad(two triangles) to represent a face, the final face number(here represent the number of triangles) should multiple 2.

	s.close();

	if (pos.size() == vNum && indices.size() == fNum)
	{
		Print("OFF " + _fileName + " has been loaded.");
		shared_ptr<Mesh> mesh = make_shared<Mesh>();
		mesh->SetPositions(pos);
		mesh->SetNormals(normals);
		mesh->SetIndices(indices);
		return mesh;
	}
	else
	{
		Print("[Error] failed to load OFF: " + _fileName);
		return nullptr;
	}
}


shared_ptr<Mesh> MeshGenerator::GenMeshFromOBJ(const std::string _fileName, vector<glm::vec2>& _uv)
{
	// [Important] TODO: pyuan-21: this function is not finished. For now it just reads vertices, normals.
	// TODO: still need to parse the .mtl, and how to organize the case that different faces group can read different texture but they are sharing the same vertices set.
	// change the rendering process.(For example, a car contains four wheels and a shell. There are two textures for wheels and shell.)
	// Also, it is very slow, and it allocates a lot of memory. To optimize it later.
	
	// Load an OBJ file. See https://en.wikipedia.org/wiki/Wavefront_.obj_file)
	std::string filePath = "Resources/Models/OBJ/" + _fileName;
	std::ifstream s(filePath);
	if (!s.is_open())
	{
		Print("[Error] failed to open: " + filePath);
		return nullptr;
	}

	std::string line;
	vector<glm::vec3> positions;
	vector<glm::vec3> normals;
	vector<glm::uvec3> indices;

	// TODO: it seems in .obj, there is no duplicated definition for data. Therefore, we need use temp.
	vector<glm::vec3> pTemp; // pos temp
	vector<glm::vec3> nTemp; // normal temp
	vector<glm::vec2> uvTemp; // uv temp

	// Each face has a lot of points(at least 3 points). Each point has vertex-index(necessary), uv-index(optional) and normal-index(optional).
	struct ObjFace
	{
		vector<glm::uvec3> points;
	};
	vector<ObjFace> faces;

	// read OBJ file
	while (std::getline(s, line))
	{
		if (line.empty())
			continue;

		// read vertex
		if (line[0] == 'v' && line[1] == ' ')
		{
			try
			{
				auto strList = Utility::SplitString(line.substr(2, line.size() - 2), " ");
				glm::vec3 v(std::stof(strList[0]), std::stof(strList[1]), std::stof(strList[2]));
				if (strList.size() > 3)
					v /= std::stof(strList[3]);
				pTemp.push_back(v);
			}
			catch (std::exception const& ex)
			{
				Print("[Error] can not read the values of vertice.");
				Print(NULL, "ss", "Exception: ", ex.what());
				return nullptr;
			}
		}

		// read uv coordinate at vertex, if it provides
		if (line[0] == 'v' && line[1] == 't')
		{
			try
			{
				auto strList = Utility::SplitString(line.substr(3, line.size() - 3), " ");
				glm::vec2 uv(std::stof(strList[0]), std::stof(strList[1]));
				uvTemp.push_back(uv);
			}
			catch (std::exception const& ex)
			{
				Print("[Error] can not read the values of uv.");
				Print(NULL, "ss", "Exception: ", ex.what());
				return nullptr;
			}
		}

		// read normal at vertex
		if (line[0] == 'v' && line[1] == 'n')
		{
			try
			{
				auto strList = Utility::SplitString(line.substr(3, line.size() - 3), " ");
				glm::vec3 n(std::stof(strList[0]), std::stof(strList[1]), std::stof(strList[2]));
				nTemp.push_back(n);
			}
			catch (std::exception const& ex)
			{
				Print("[Error] can not read the values of normal.");
				Print(NULL, "ss", "Exception: ", ex.what());
				return nullptr;
			}
		}

		// read faces, which can be "vertex1 vertex2 vertex3"(triangle) or "vertex1 vertex2 vertex3 vertex4"(quad), or more vertices on a face.
		// but anyway, these vertices can be always represented by many triangles like this way: "vertex1-vertex2-vertex3", "vertex1-vertex3-vertex4", "vertex1-vertex4-vertex5", and etc.
		// Among them, the vertex1 is always the shared point for other vertices to construct a triangle.
		// each vertex can have "vertex_index/texture_index/normal_index" or just "vertex_index//normal_index"
		// and index is starting from 1 not 0.
		if (line[0] == 'f' && line[1] == ' ')
		{
#pragma region Case Determination
			// we have four case: vertex, vertex/texture, vertex//normal, vertex/texture/normal.
			// by checking the first element to determine which case we are:
			int caseType = 0;
			size_t startPos = std::string::npos;
			size_t endPos = std::string::npos;

			endPos = line.find(' ', 2);
			std::string firstElement = line.substr(2, endPos - 2);

			if (firstElement.find("/") == std::string::npos)
			{
				// case: vertex
				caseType = 1;
			}
			else
			{
				if (firstElement.find("//") == std::string::npos)
				{
					if (firstElement.find("/", firstElement.find("/") + 1) == std::string::npos)
					{
						// case: vertex/texture
						caseType = 2;
					}
					else
					{
						// case: vertex/texture/normal
						caseType = 3;
					}
				}
				else
				{
					// case: vertex//normal
					caseType = 4;
				}
			}
#pragma endregion

			ObjFace newFace;

#pragma region Prepare the point data for each face
			switch (caseType)
			{
			case 1: // vertex
			{
				size_t vertexIndex = std::stoull(firstElement);
				newFace.points.push_back({ vertexIndex, 0, 0 }); // 0 means no specify.
				while (endPos != std::string::npos && endPos <= line.size() - 2)
				{
					startPos = endPos + 1;
					endPos = line.find(' ', startPos);

					vertexIndex = std::stoull(line.substr(startPos, endPos - startPos));
					newFace.points.push_back({ vertexIndex, 0, 0 }); // 0 means no specify.
				}
			}
			break;
			case 2: // vertex/texture
			{
				auto strList = Utility::SplitString(firstElement, "/");
				size_t vertexIndex, uvIndex;
				vertexIndex = std::stoull(strList[0]);
				uvIndex = std::stoull(strList[1]);
				newFace.points.push_back({ vertexIndex, uvIndex, 0 }); // 0 means no specify.
				while (endPos != std::string::npos && endPos <= line.size() - 2)
				{
					startPos = endPos + 1;
					endPos = line.find(' ', startPos);

					auto tempStr = line.substr(startPos, endPos - startPos);
					auto strList = Utility::SplitString(tempStr, "/");
					vertexIndex = std::stoull(strList[0]);
					uvIndex = std::stoull(strList[1]);
					newFace.points.push_back({ vertexIndex, uvIndex, 0 }); // 0 means no specify.
				}
			}
			break;
			case 3: // vertex/texture/normal
			{
				auto strList = Utility::SplitString(firstElement, "/");
				size_t vertexIndex, uvIndex, normalIndex;
				vertexIndex = std::stoull(strList[0]);
				uvIndex = std::stoull(strList[1]);
				normalIndex = std::stoull(strList[2]);
				newFace.points.push_back({ vertexIndex, uvIndex, normalIndex });
				while (endPos != std::string::npos && endPos <= line.size() - 2)
				{
					startPos = endPos + 1;
					endPos = line.find(' ', startPos);

					auto tempStr = line.substr(startPos, endPos - startPos);
					auto strList = Utility::SplitString(tempStr, "/");
					vertexIndex = std::stoull(strList[0]);
					uvIndex = std::stoull(strList[1]);
					normalIndex = std::stoull(strList[2]);
					newFace.points.push_back({ vertexIndex, uvIndex, normalIndex });
				}
			}
			break;
			case 4: // vertex//normal
			{
				auto strList = Utility::SplitString(firstElement, "/");
				size_t vertexIndex, normalIndex;
				vertexIndex = std::stoull(strList[0]);
				normalIndex = std::stoull(strList[1]);
				newFace.points.push_back({ vertexIndex, 0, normalIndex }); // 0 means no specify.
				while (endPos != std::string::npos && endPos <= line.size() - 2)
				{
					startPos = endPos + 1;
					endPos = line.find(' ', startPos);

					auto tempStr = line.substr(startPos, endPos - startPos);
					auto strList = Utility::SplitString(tempStr, "/");
					vertexIndex = std::stoull(strList[0]);
					normalIndex = std::stoull(strList[1]);
					newFace.points.push_back({ vertexIndex, 0, normalIndex }); // 0 means no specify.
				}
			}
			break;
			default:
				Print("[Error] Can not determine the case type when loading an OBJ: " + _fileName);
				return nullptr;
			}
#pragma endregion

			faces.push_back(newFace);
		}
	}

	s.close();

	// TODO: in fact we should keep it as it is. (E.g. if the model is built with quad, we should use glDrawElements to draw quad directly and save quad indices.)
	// For the time being, I just use triangles.
	
	// Be careful, the vertices, uv, normals stored in .obj file can be compressed(less size than the size of "positions" which is what we expect to store in Mesh)

	auto updateFunc = [&positions, &normals, &_uv, &pTemp, &uvTemp, &nTemp](glm::uvec3 &point)
	{
		// each "point" contains "vertex_index/texture_index/normal_index", 0 means no data because this index is starting from 1.
		positions.push_back(pTemp[point[0] - 1]); // vertex_index(point[0]) is starting from 1 instead of 0.

		// update the _uv/noramls data for point. If index is equal to 0, it means no information.
		if (point[1] != 0)
			_uv.push_back(uvTemp[point[1] - 1]); // texture_index

		if (point[2] != 0)
			normals.push_back(nTemp[point[2] - 1]); // normal_index

		return positions.size() - 1;
	};

	// using the faces to construct a lot of triangles.
	for (auto face : faces)
	{
		glm::uvec3 point0 = face.points[0]; // each point contains "vertex_index/texture_index/normal_index", 0 means no data because this index is starting from 1.

		size_t triIndex1 = updateFunc(point0); // we don't change the first index of triangle.

		size_t triIndex2, triIndex3;

		for (size_t point2Index = 2; point2Index < face.points.size(); point2Index++)
		{
			glm::uvec3 point1 = face.points[point2Index - 1];
			triIndex2 = updateFunc(point1);
			
			glm::uvec3 point2 = face.points[point2Index];
			triIndex3 = updateFunc(point2);

			// add this triangle
			glm::uvec3 triangleIndex = glm::uvec3(triIndex1, triIndex2, triIndex3);
			indices.push_back(triangleIndex);
		}
	}

	// verify data
	if (positions.size() > 0 && indices.size() > 0)
	{
		Print("OBJ " + _fileName + " has been loaded.");
		if(normals.size()<=0)
			Print("[Warning] OBJ " + _fileName + " has no \"normals\" information.");

		shared_ptr<Mesh> mesh = make_shared<Mesh>();
		mesh->SetPositions(positions);
		mesh->SetNormals(normals);
		mesh->SetIndices(indices);
		return mesh;
	}
	else
	{
		Print("[Error] failed to load OBJ: " + _fileName);
		return nullptr;
	}
}