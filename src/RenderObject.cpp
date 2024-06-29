#include "RenderObject.h"
#include "DeviceAllocator.h"
#include "Image.h"
#include "Mesh.h"
#include "Material.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan_core.h>
#include <spdlog/spdlog.h>

RenderObject::RenderObject(DeviceAllocator &allocator, const Mesh &mesh, const Material &material, std::string name)
    : transform(1.f), 
    indexCount(mesh.getIndexCount()),
    vertexCount(mesh.getVertexCount()),
    indexType(mesh.getIndexType()),
    allocator(allocator),
    vertexBuffer(
        allocator, 
        (void *) mesh.getVertexData().data(), 
        mesh.getVertexDataSize(), 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    ),
    indexBuffer(
        indexCount > 0 
        ? std::make_unique<Buffer>(
            allocator, 
            (void *) mesh.getIndexData().data(), 
            mesh.getIndexDataSize(), 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        : nullptr
    ),
    material(material),
    name(name)
{
}

RenderObject::RenderObject(RenderObject &&other)
    : transform(other.transform),
    indexCount(other.indexCount),
    vertexCount(other.vertexCount),
    indexType(other.indexType),
    allocator(other.allocator),
    vertexBuffer(std::move(other.vertexBuffer)),
    indexBuffer(std::move(other.indexBuffer)),
    material(other.material),
    name(std::move(other.name))
{
}

RenderObject::~RenderObject()
{
    indexBuffer.reset();
}

const glm::mat4 &RenderObject::getTransform() const
{
    return transform;
}

void RenderObject::setTransform(const glm::mat4 &transform)
{
    this->transform = transform;
}

const Material &RenderObject::getMaterial() const
{
    return material;
}

void RenderObject::enqueueDrawCommands(VkCommandBuffer commandBuffer) const
{
    VkBuffer vertexBuffers[] = {vertexBuffer.getHandle()};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    if (indexCount > 0) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getHandle(), 0, indexType);

        vkCmdDrawIndexed(
            commandBuffer, 
            indexCount, 
            1, 
            0, 
            0, 
            0
        );
    }
    else {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
}
