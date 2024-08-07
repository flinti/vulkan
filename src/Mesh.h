#ifndef MESH_H_
#define MESH_H_

#include "Vertex.h"

#include <cstdint>
#include <glm/fwd.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

class Mesh
{
public:
    typedef uint32_t IndexType;

    const std::vector<Vertex> &getVertexData() const;
    size_t getVertexDataSize() const;
    uint32_t getVertexCount() const;
    const std::vector<IndexType> &getIndexData() const;
    size_t getIndexDataSize() const;
    uint32_t getIndexCount() const;
    VkIndexType getIndexType() const;

    static Mesh createRegularPolygon(float r, uint32_t edges, glm::vec3 offset = glm::vec3(0.f));
    static Mesh createPlane(glm::vec3 a, glm::vec3 b, glm::vec3 offset = glm::vec3(0.f));
    static Mesh createTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 offset = glm::vec3(0.f));
    static Mesh createUnitCube();
private:
    std::vector<Vertex> vertices;
    std::vector<IndexType> indices;
};

#endif
