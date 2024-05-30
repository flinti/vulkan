#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>
#include <stdexcept>
#include <spdlog/fmt/fmt.h>

#define VK_ASSERT(x) do { \
    VkResult result = x; \
    if (result != VK_SUCCESS) { \
        throw std::runtime_error(fmt::format("vulkan error {}: " #x, string_VkResult(result))); \
    } \
} while(0)
