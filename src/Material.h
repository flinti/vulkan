#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "Buffer.h"
#include "DeviceAllocator.h"
#include "GraphicsPipeline.h"
#include "Image.h"
#include "ResourceRepository.h"
#include <cstdint>
#include <filesystem>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <vulkan/vulkan_core.h>

class Device;

class Material
{
public:
    struct Parameters
    {
        glm::vec3 ambient;
        float pad1;
        glm::vec3 diffuse;
        float pad2;
        glm::vec4 specularAndShininess;
    };

    Material(
        uint32_t id,
        Device &device,
        const ShaderResource &vertexShader,
        const ShaderResource &fragmentShader,
        const std::vector<ImageResource> &imageResources,
        const Parameters &parameters,
        std::string name = ""
    );
    Material(const Material &) = delete;
    Material(Material &&other);
    ~Material();

    uint32_t getId() const;
    const ShaderResource &getVertexShaderResource() const;
    const ShaderResource &getFragmentShaderResource() const;
    VkSampler getSamplerHandle() const;
    const std::vector<VkDescriptorSetLayoutBinding> &getDescriptorSetLayoutBindings() const;
    const std::map<uint32_t, VkDescriptorImageInfo> &getDescriptorImageInfos() const;
    const std::map<uint32_t, VkDescriptorBufferInfo> &getDescriptorBufferInfos() const;
private:
    std::vector<Image> createImages(const std::vector<ImageResource> &imageResources);
    std::vector<VkImageView>  createImageViews();
    VkSampler requestSampler();
    std::vector<VkDescriptorSetLayoutBinding> createDescriptorSetLayoutBindings();
    std::map<uint32_t, VkDescriptorImageInfo> createDescriptorImageInfos();
    std::map<uint32_t, VkDescriptorBufferInfo> createDescriptorBufferInfos();

    uint32_t id;
    Device &device;
    const ShaderResource &vertexShader;
    const ShaderResource &fragmentShader;
    std::vector<Image> images;
    std::vector<VkImageView> imageViews;
    VkSampler sampler = VK_NULL_HANDLE;
    Buffer parameterBuffer;
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    std::map<uint32_t, VkDescriptorImageInfo> descriptorImageInfos;
    std::map<uint32_t, VkDescriptorBufferInfo> descriptorBufferInfos;
    std::string name;
};

#endif
