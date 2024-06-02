#ifndef RENDEROBJECT_H_
#define RENDEROBJECT_H_

#include "Buffer.h"
#include "DeviceAllocator.h"
#include <string>
#include <vulkan/vulkan_core.h>
#include <glm/mat4x4.hpp>

class Mesh;

class RenderObject
{
public:
    RenderObject(DeviceAllocator &allocator, const Mesh &mesh, std::string name = "");
    RenderObject(const RenderObject &) = delete;
    RenderObject(RenderObject &&) = default;
    ~RenderObject();

    const glm::mat4 &getTransform() const;
    void setTransform(const glm::mat4 &transform);

    void enqueueDrawCommands(VkCommandBuffer commandBuffer)const ;
private:
    glm::mat4 transform;
    size_t indexCount;
    VkIndexType indexType;
    DeviceAllocator &allocator;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    std::string name;
};

#endif
