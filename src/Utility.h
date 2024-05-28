#ifndef UTILITY_H_
#define UTILITY_H_

#include <vector>
#include <filesystem>

namespace Utility
{
    std::vector<std::byte> readFile(std::filesystem::path path);
};

#endif
