#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "DeviceAllocator.h"
#include "Image.h"
#include <filesystem>
#include <string>

class Material
{
public:
    Material(
        DeviceAllocator &allocator, 
        VkDevice device, 
        const std::filesystem::path &imageFile, 
        std::string name = ""
    );
    Material(const Material &) = delete;
    Material(Material &&other);
    ~Material();

    VkImage getImageHandle() const;
    VkImageView getImageViewHandle() const;
    VkSampler getSamplerHandle() const;
private:
    VkImageView createImageView();
    VkSampler createSampler();

    DeviceAllocator &allocator;
    VkDevice device;
    Image image;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    // TODO: Pipeline and proper descriptor management etc.
    std::string name;
};

#endif
