#include "RenderObject.h"
#include "Device.h"
#include "Mesh.h"
#include "Material.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan_core.h>
#include <spdlog/spdlog.h>

RenderObject::RenderObject(Device &device, const Mesh &mesh, const Material &material, std::string name)
    : device(device),
    material(material),
    transform(1.f), 
    indexCount(mesh.getIndexCount()),
    vertexCount(mesh.getVertexCount()),
    indexType(mesh.getIndexType()),
    vertexBuffer(
        device.getAllocator(), 
        (void *) mesh.getVertexData().data(), 
        mesh.getVertexDataSize(), 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    ),
    indexBuffer(
        indexCount > 0 
        ? std::make_unique<Buffer>(
            device.getAllocator(), 
            (void *) mesh.getIndexData().data(), 
            mesh.getIndexDataSize(), 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        : nullptr
    ),
    name(name)
{
}

RenderObject::RenderObject(RenderObject &&other)
    : device(other.device),
    material(other.material),
    transform(other.transform),
    indexCount(other.indexCount),
    vertexCount(other.vertexCount),
    indexType(other.indexType),
    vertexBuffer(std::move(other.vertexBuffer)),
    indexBuffer(std::move(other.indexBuffer)),
    name(std::move(other.name))
{
}

RenderObject::~RenderObject()
{
    indexBuffer.reset();
}

std::vector<VkDescriptorSetLayoutBinding> RenderObject::getGlobalUniformDataLayoutBindings()
{
	return std::vector<VkDescriptorSetLayoutBinding>{
		VkDescriptorSetLayoutBinding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		},
	};
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
