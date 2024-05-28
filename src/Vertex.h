#ifndef VERTEX_H_
#define VERTEX_H_

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

#endif
