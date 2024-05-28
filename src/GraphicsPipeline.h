#ifndef GRAPHICS_PIPELINE_H_
#define GRAPHICS_PIPELINE_H_

#include <vulkan/vulkan_core.h>
#include <vector>

class RenderPass;

class GraphicsPipeline
{
public:
    GraphicsPipeline(VkDevice device, const RenderPass &renderPass);
    ~GraphicsPipeline();

    void bind(VkCommandBuffer commandBuffer);
private:
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<std::byte> &shader);

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkDevice device;
    const RenderPass &renderPass;
};

#endif
