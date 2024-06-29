#ifndef GRAPHICS_PIPELINE_H_
#define GRAPHICS_PIPELINE_H_

#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"

#include <vulkan/vulkan_core.h>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class RenderPass;
class DescriptorSet;

struct PushConstants {
    glm::mat4 transform;
    glm::vec4 time;
};

class GraphicsPipeline
{
public:
    GraphicsPipeline(VkDevice device, const RenderPass &renderPass);
    ~GraphicsPipeline();

    const DescriptorSetLayout &getDescriptorSetLayout() const;
    void bind(VkCommandBuffer commandBuffer);
    void bindDescriptorSet(VkCommandBuffer commandBuffer, const DescriptorSet &set);
    void pushConstants(VkCommandBuffer commandBuffer, const void *data, size_t size);
private:
    std::vector<VkDescriptorSetLayoutBinding> getDescriptorSetLayoutBindings();
    void createPipelineLayout();
    VkShaderModule createShaderModule(const std::vector<std::byte> &shader);
    void createGraphicsPipeline();

    DescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkDevice device;
    const RenderPass &renderPass;
};

#endif
