#ifndef MESH_H_
#define MESH_H_

#include "Vertex.h"

#include <cstdint>
#include <vector>

class Mesh
{
public:
    const std::vector<Vertex> &getVertexData() const;
    size_t getVertexDataSize() const;
    const std::vector<uint16_t> &getIndexData() const;
    size_t getIndexDataSize() const;

    static Mesh createRegularPolygon(float r, uint32_t edges);
private:
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

#endif
