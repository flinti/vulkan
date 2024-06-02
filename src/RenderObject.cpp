#include "RenderObject.h"
#include "DeviceAllocator.h"
#include "Mesh.h"
#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan_core.h>

RenderObject::RenderObject(DeviceAllocator &allocator, const Mesh &mesh, std::string name)
    : transform(1.f), 
    indexCount(mesh.getIndexCount()),
    indexType(mesh.getIndexType()),
    allocator(allocator),
    vertexBuffer(
        allocator, 
        (void *) mesh.getVertexData().data(), 
        mesh.getVertexDataSize(), 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    ),
    indexBuffer(
        allocator, 
        (void *) mesh.getIndexData().data(), 
        mesh.getIndexDataSize(), 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    ),
    name(name)
{
}

RenderObject::~RenderObject()
{
}

const glm::mat4 &RenderObject::getTransform() const
{
    return transform;
}

void RenderObject::setTransform(const glm::mat4 &transform)
{
    this->transform = transform;
}

void RenderObject::enqueueDrawCommands(VkCommandBuffer commandBuffer) const
{
    VkBuffer vertexBuffers[] = {vertexBuffer.getHandle()};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer.getHandle(), 0, indexType);

	vkCmdDrawIndexed(
		commandBuffer, 
		indexCount, 
		1, 
		0, 
		0, 
		0
	);
}
