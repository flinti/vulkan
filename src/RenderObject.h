#ifndef RENDEROBJECT_H_
#define RENDEROBJECT_H_

#include "Buffer.h"
#include "Image.h"
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>
#include <glm/mat4x4.hpp>

class Mesh;
class Material;
class Device;

class RenderObject
{
public:
    RenderObject(Device &device, const Mesh &mesh, const Material &material, std::string name = "");
    RenderObject(const RenderObject &) = delete;
    RenderObject(RenderObject &&);
    ~RenderObject();

    const glm::mat4 &getTransform() const;
    void setTransform(const glm::mat4 &transform);
    const Material &getMaterial() const;

    void enqueueDrawCommands(VkCommandBuffer commandBuffer)const ;
private:
    Device &device;
    const Material &material;
    glm::mat4 transform;
    uint32_t indexCount;
    uint32_t vertexCount;
    VkIndexType indexType;
    Buffer vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;
    std::string name;
};

#endif
