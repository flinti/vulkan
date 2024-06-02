#ifndef VERTEX_H_
#define VERTEX_H_

#include <array>
#include <iostream>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>


struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 uv;

    std::string to_string() const;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
};

std::ostream &operator <<(std::ostream &s, const Vertex &v);

#endif
