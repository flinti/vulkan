#ifndef RENDEROBJECT_H_
#define RENDEROBJECT_H_

#include "Buffer.h"
#include "DeviceAllocator.h"
#include "Image.h"
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>
#include <glm/mat4x4.hpp>

class Mesh;
class Material;

class RenderObject
{
public:
    RenderObject(DeviceAllocator &allocator, const Mesh &mesh, const Material &material, std::string name = "");
    RenderObject(const RenderObject &) = delete;
    RenderObject(RenderObject &&);
    ~RenderObject();

    const glm::mat4 &getTransform() const;
    void setTransform(const glm::mat4 &transform);
    const Material &getMaterial() const;

    void enqueueDrawCommands(VkCommandBuffer commandBuffer)const ;
private:
    glm::mat4 transform;
    uint32_t indexCount;
    uint32_t vertexCount;
    VkIndexType indexType;
    DeviceAllocator &allocator;
    Buffer vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;
    const Material &material;
    std::string name;
};

#endif
