#include "Mesh.h"
#include "Utility.h"
#include <glm/detail/qualifier.hpp>
#include <glm/fwd.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>


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

size_t Mesh::getIndexCount() const
{
	return indices.size();
}


VkIndexType Mesh::getIndexType() const
{
	return VK_INDEX_TYPE_UINT16;
}

Mesh Mesh::createRegularPolygon(float r, uint32_t edges, glm::vec3 offset)
{
    Mesh mesh;
	mesh.vertices.reserve(edges + 1);
	mesh.indices.reserve(edges * 3);

	for (uint32_t i = 0; i < edges; ++i) {
		float phi = 2 * 3.1415926f / edges * i;
		float x = r * glm::cos(phi);
		float z = r * glm::sin(phi);

		Vertex v{
			.position = { x + offset.x, offset.y, z + offset.z },
			.color = Utility::colorFromHsl(360.f / edges * i, 1.f, 0.5f),
			.uv = { 0.f, 0.f },
		};
		mesh.vertices.push_back(v);
		mesh.indices.push_back(edges);
		mesh.indices.push_back(i);
		mesh.indices.push_back((i + 1) % edges);
	}
	mesh.vertices.emplace_back(Vertex{
		.position = offset, 
		.color = { 0.f, 0.f, 0.f },
		.uv = { 0.f, 0.f },
	});

    return mesh;
}

Mesh Mesh::createPlane(glm::vec3 a, glm::vec3 b, glm::vec3 offset)
{
	Mesh mesh;
	mesh.vertices = {
		{
			.position = offset, 
			.color = { 1.f, 0.f, 0.f },
			.uv = { 0.f, 0.f },
		},
		{
			.position = offset + a, 
			.color = { 0.f, 1.f, 0.f },
			.uv = { 0.f, 0.f },
		},
		{
			.position = offset + a + b, 
			.color = { 0.f, 0.f, 1.f },
			.uv = { 0.f, 0.f },
		},
		{
			.position = offset + b, 
			.color = { 1.f, 1.f, 1.f },
			.uv = { 0.f, 0.f },
		},
	};
	mesh.indices = { 0, 1, 2, 2, 3, 0};

	return mesh;
}

Mesh Mesh::createTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 offset)
{
	Mesh mesh;
	mesh.vertices = {
		{
			.position = offset, 
			.color = { 1.f, 0.f, 0.f },
			.uv = { 0.f, 0.f },
		},
		{
			.position = offset + a, 
			.color = { 0.f, 1.f, 0.f },
			.uv = { 0.f, 0.f },
		},
		{
			.position = offset + a + b, 
			.color = { 0.f, 0.f, 1.f },
			.uv = { 0.f, 0.f },
		},
	};
	mesh.indices = { 0, 1, 2 };

	return mesh;
}

Mesh Mesh::createUnitCube()
{
	Mesh mesh;
	mesh.vertices = {
		{ .position = {-0.5f, 0.5f, 0.5f }, .color = { 0.f, 1.f, 0.f } },
		{ .position = {-0.5f, -0.5f, 0.5f }, .color = { 0.f, 1.f, 0.f } },
		{ .position = {0.5f, 0.5f, 0.5f }, .color = { 1.f, 0.f, 0.f } },
		{ .position = {0.5f, -0.5f, 0.5f }, .color = { 0.f, 1.f, 0.f } },
		{ .position = {-0.5f, 0.5f, -0.5f }, .color = { 0.f, 1.f, 0.f } },
		{ .position = {-0.5f, -0.5f, -0.5f }, .color = { 0.f, 0.f, 1.f } },
		{ .position = {0.5f, 0.5f, -0.5f }, .color = { 0.f, 1.f, 0.f } },
		{ .position = {0.5f, -0.5f, -0.5f }, .color = { 0.f, 1.f, 0.f } }
	};
	mesh.indices = {
		0, 2, 3, 0, 3, 1,
		2, 6, 7, 2, 7, 3,
		6, 4, 5, 6, 5, 7,
		4, 0, 1, 4, 1, 5,
		0, 4, 6, 0, 6, 2,
		1, 5, 7, 1, 7, 3,
	};

	return mesh;
}
