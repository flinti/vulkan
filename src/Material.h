#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "DeviceAllocator.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "ResourceRepository.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <vulkan/vulkan_core.h>

class Device;

struct GlobalUniformData
{
    glm::mat4 viewProj;
    glm::vec3 viewPos;
    float pad1;
    glm::vec4 time;
    glm::vec3 lightPosition;
    float pad2;
    glm::vec3 lightColor;
    float pad3;
};

class Material
{
public:
    Material(
        uint32_t id,
        Device &device,
        const ShaderResource &vertexShader,
        const ShaderResource &fragmentShader,
        const ImageResource &imageResource,
        std::string name = ""
    );
    Material(const Material &) = delete;
    Material(Material &&other);
    ~Material();

    uint32_t getId() const;
    const ShaderResource &getVertexShaderResource() const;
    const ShaderResource &getFragmentShaderResource() const;
    VkImage getImageHandle() const;
    VkImageView getImageViewHandle() const;
    VkSampler getSamplerHandle() const;
    const std::vector<VkDescriptorSetLayoutBinding> &getDescriptorSetLayoutBindings() const;
    const std::map<uint32_t, VkDescriptorImageInfo> &getDescriptorImageInfos() const;
private:
    VkImageView createImageView();
    VkSampler requestSampler();
    std::vector<VkDescriptorSetLayoutBinding> createDescriptorSetLayoutBindings();
    std::map<uint32_t, VkDescriptorImageInfo> createDescriptorImageInfos();

    uint32_t id;
    Device &device;
    const ShaderResource &vertexShader;
    const ShaderResource &fragmentShader;
    Image image;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    std::map<uint32_t, VkDescriptorImageInfo> descriptorImageInfos;
    std::string name;
};

#endif
