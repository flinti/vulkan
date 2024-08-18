#ifndef UTILITY_H_
#define UTILITY_H_

#include <glm/glm.hpp>
#include <vector>
#include <filesystem>

namespace Utility
{
    std::vector<std::byte> readFile(std::filesystem::path path);
    glm::vec3 colorFromHsl(float H, float S, float L);

    template <class T>
    inline void hash_combine(size_t &seed, const T &v)
    {
        std::hash<T> hasher;
        size_t hash = hasher(v);
        hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash;
    }

    template <class T>
    inline size_t hash_value(const T &v)
    {
        size_t seed = 0;
        hash_combine(seed, v);
        return seed;
    }
};

#endif
