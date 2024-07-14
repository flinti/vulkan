#ifndef GRAPHICS_PIPELINE_H_
#define GRAPHICS_PIPELINE_H_

#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "ResourceRepository.h"

#include <vulkan/vulkan_core.h>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class RenderPass;
class DescriptorSet;
class Material;
class Device;

struct PushConstants {
    glm::mat4 transform;
    glm::vec4 time;
};

class GraphicsPipeline
{
public:
    GraphicsPipeline(
        Device &device,
        const RenderPass &renderPass,
        const Material &material
    );
    ~GraphicsPipeline();

    const DescriptorSetLayout &getDescriptorSetLayout() const;
    const Material &getMaterial() const;
    void bind(VkCommandBuffer commandBuffer);
    void bindDescriptorSet(VkCommandBuffer commandBuffer, const DescriptorSet &set);
    void pushConstants(VkCommandBuffer commandBuffer, const void *data, size_t size);
private:
    void createPipelineLayout();
    VkShaderModule createShaderModule(const std::vector<std::byte> &shader);
    void createGraphicsPipeline(const ShaderResource &vertexShader, const ShaderResource &fragmentShader);

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    Device &device;
    const RenderPass &renderPass;
    const Material &material;
    DescriptorSetLayout &descriptorSetLayout;
};

#endif
