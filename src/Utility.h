#ifndef UTILITY_H_
#define UTILITY_H_

#include <glm/glm.hpp>
#include <vector>
#include <filesystem>

namespace Utility
{
    std::vector<std::byte> readFile(std::filesystem::path path);
    glm::vec3 colorFromHsl(float H, float S, float L);
};

#endif
