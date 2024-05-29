#include "Mesh.h"
#include "Utility.h"


const std::vector<Vertex> &Mesh::getVertexData() const
{
    return vertices;
}

size_t Mesh::getVertexDataSize() const
{
	return vertices.size() * sizeof(vertices[0]);
}

const std::vector<uint16_t> &Mesh::getIndexData() const
{
    return indices;
}

size_t Mesh::getIndexDataSize() const
{
	return indices.size() * sizeof(indices[0]);
}

Mesh Mesh::createRegularPolygon(float r, uint32_t edges)
{
    Mesh mesh;
	mesh.vertices.reserve(edges + 1);
	mesh.indices.reserve(edges + 2);
	mesh.vertices = {
        {
				.position = { 0.f, 0.f, 0.f }, 
				.color = { 1.f, 1.f, 1.f },
				.uv = { 0.f, 0.f },
			}
    };
	mesh.indices = {  0  };
	for (uint32_t i = 0; i < edges; ++i) {
		float phi = 2 * 3.1415926f / edges * i;
		float x = r * glm::cos(phi);
		float z = r * glm::sin(phi);

		Vertex v{
			.position = { x, 0.f, z },
			.color = Utility::colorFromHsl(360.f / edges * i, 1.f, 0.5f),
			.uv = { 0.f, 0.f },
		};
		mesh.vertices.push_back(v);
		mesh.indices.push_back(i + 1);
	}
	mesh.indices.push_back(1);

    return mesh;
}
