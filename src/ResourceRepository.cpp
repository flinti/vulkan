#include "ResourceRepository.h"
#include "Utility.h"
#include "third-party/stb_image.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <spdlog/spdlog.h>
#include <vector>

using namespace std::filesystem;

ResourceRepository::ResourceRepository()
{
    loadAll();
}

ResourceRepository::~ResourceRepository()
{
    for (auto &i : images) {
        stbi_image_free(i.second.data);
    }
}

const Mesh &ResourceRepository::getMesh(const ResourceKey &name) const
{
    const auto &i = meshes.find(name);
    if (i == meshes.end()) {
        throw std::runtime_error(fmt::format("ResourceRepository::get*: Resource {} does not exist", name));
    }
    return i->second;
}

const ImageResource &ResourceRepository::getImage(const ResourceKey &name) const
{
    const auto &i = images.find(name);
    if (i == images.end()) {
        throw std::runtime_error(fmt::format("ResourceRepository::get*: Resource {} does not exist", name));
    }
    return i->second;
}

const ShaderResource &ResourceRepository::getFragmentShader(const ResourceKey &name) const
{
    const auto &i = fragmentShaders.find(name);
    if (i == fragmentShaders.end()) {
        throw std::runtime_error(fmt::format("ResourceRepository::get*: Resource {} does not exist", name));
    }
    return i->second;
}

const ShaderResource &ResourceRepository::getVertexShader(const ResourceKey &name) const
{
    const auto &i = vertexShaders.find(name);
    if (i == vertexShaders.end()) {
        throw std::runtime_error(fmt::format("ResourceRepository::get*: Resource {} does not exist", name));
    }
    return i->second;
}

bool ResourceRepository::insertMesh(const ResourceKey &name, Mesh mesh)
{
    return meshes.insert({name, std::move(mesh)}).second;
}

void ResourceRepository::loadImage(const ResourceKey &name, const std::filesystem::path &path)
{
    int wdt;
    int hgt;
    int channels;
    auto *imageData = stbi_load(
        path.c_str(), 
        &wdt, 
        &hgt, 
        &channels, 
        STBI_rgb_alpha
    );

    if (!imageData) {
        throw std::runtime_error(fmt::format("Failed to load image {}", name));
    }

    images.emplace(name, ImageResource{
        (uint32_t) wdt,
        (uint32_t) hgt,
        (void *) imageData
    });
}

void ResourceRepository::loadFragmentShader(const ResourceKey &name, const std::filesystem::path &path)
{
    spdlog::info("Loading fragment shader {} ", path.string());
    fragmentShaders.emplace(name, readShaderFile(path));
}

void ResourceRepository::loadVertexShader(const ResourceKey &name, const std::filesystem::path &path)
{
    spdlog::info("Loading vertex shader {} ", path.string());
    vertexShaders.emplace(name, readShaderFile(path));
}

std::string ResourceRepository::resourceTree(size_t indentationLevel) const
{
    size_t count = meshes.size()
        + images.size()
        + vertexShaders.size()
        + fragmentShaders.size();
    std::vector<std::string> keys;
    keys.reserve(count);

    for (const auto &i : meshes) {
        keys.push_back(i.first);
    }
    for (const auto &i : images) {
        keys.push_back(i.first);
    }
    for (const auto &i : vertexShaders) {
        keys.push_back(i.first);
    }
    for (const auto &i : fragmentShaders) {
        keys.push_back(i.first);
    }

    std::sort(keys.begin(), keys.end());

    std::string output;
    std::string indentationPrefix = std::string(indentationLevel, '\t');
    for (const auto &k : keys) {
        output += indentationPrefix + k + "\n";
    }

    return output;
}

void ResourceRepository::load(const directory_entry &dirEntry)
{
    std::string resourceName = dirEntry.path().lexically_relative(current_path()).generic_string();
    std::string extension = dirEntry.path().extension();
    const path &loadPath = dirEntry.path();
    try {
        if (extension == ".png") {
            loadImage(resourceName, loadPath);
        }
        else if(extension == ".frag") {
            loadFragmentShader(resourceName, loadPath);
        }
        else if(extension == ".vert") {
            loadVertexShader(resourceName, loadPath);
        }
        else {
            spdlog::warn("No loader for resource {}", resourceName);
        }
    }
    catch (std::exception &e) {
        spdlog::error("ResourceRepository: Loading resource {} failed: {}!", resourceName, e.what());
    }
}

void ResourceRepository::loadAll()
{
    auto currentPath = current_path();
    recursive_directory_iterator iterator(currentPath);

    for (auto &i : iterator) {
        if (i.is_regular_file()) {
            load(i);
        }
    }
}

std::vector<std::byte> ResourceRepository::readShaderFile(const std::filesystem::path &path)
{
    {
        std::array<char, 4> magicNumber{0, 0, 0, 0};
        std::fstream file(path, std::ios::in);
        file.read(magicNumber.data(), magicNumber.size());
        // SPIR-V magic number 0x07230203
        if (magicNumber[0] != 0x03 || magicNumber[1] != 0x02 || magicNumber[2] != 0x23 || magicNumber[3] != 0x07) {
            throw std::runtime_error("Shader is not in SPIR-V format");
        }
    }
    return Utility::readFile(path);
}
