#ifndef VULKANOBJECTCACHE_H_
#define VULKANOBJECTCACHE_H_

#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class VulkanObjectCache
{
public:
    typedef size_t KeyType;

    VulkanObjectCache(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    VulkanObjectCache(const VulkanObjectCache &) = delete;
    ~VulkanObjectCache();

    VkSampler getSampler(const VkSamplerCreateInfo &info);
    DescriptorSetLayout &getDescriptorSetLayout(
        const std::vector<VkDescriptorSetLayoutBinding> &bindings,
        VkDescriptorSetLayoutCreateFlags flags = 0
    );
private:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    std::unordered_map<KeyType, VkSampler> samplers;
    std::unordered_map<KeyType, std::unique_ptr<DescriptorSetLayout>> descriptorSetLayouts;
};

#endif
