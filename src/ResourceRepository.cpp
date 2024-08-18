#include "ResourceRepository.h"
#include "Material.h"
#include "Mesh.h"
#include "Utility.h"
#include "Vertex.h"
#include "third-party/stb_image.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <stdexcept>
#include <string>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace std::filesystem;

ResourceRepository::ResourceRepository(const ResourceKey &defaultImage)
{
    loadAll();
    const auto &imageIter = images.find(defaultImage);
    if (imageIter != images.end()) {
        this->defaultImage = &imageIter->second;
    }
}

ResourceRepository::~ResourceRepository()
{
    for (auto &i : images) {
        stbi_image_free(i.second.data);
    }
}


bool ResourceRepository::hasImage(const ResourceKey &name) const
{
    return images.find(name) != images.end();
}

const Mesh &ResourceRepository::getMesh(const ResourceKey &name) const
{
    const auto &i = meshes.find(name);
    if (i == meshes.end()) {
        if (defaultMesh == nullptr) {
            throw std::runtime_error(fmt::format(
                "ResourceRepository::get*: Resource {} does not exist", name
            ));
        }
        else {
            return *defaultMesh;
        }
    }
    return i->second;
}

const MaterialResource &ResourceRepository::getMaterial(const ResourceKey &name) const
{
    const auto &i = materials.find(name);
    if (i == materials.end()) {
        throw std::runtime_error(fmt::format(
            "ResourceRepository::get*: Resource {} does not exist", name
        ));
    }
    return i->second;
}

const ImageResource &ResourceRepository::getImage(const ResourceKey &name) const
{
    const auto &i = images.find(name);
    if (i == images.end()) {
        if (defaultImage == nullptr) {
            throw std::runtime_error(fmt::format(
                "ResourceRepository::get*: Resource {} does not exist", name
            ));
        }
        else {
            return *defaultImage;
        }
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

void ResourceRepository::loadObj(const ResourceKey &name, const std::filesystem::path &path)
{
    spdlog::info("Loading .obj object {} ", path.string());

    tinyobj::attrib_t attrib{};
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;
    std::string mtlBasePath = path.parent_path().native();
    if (!mtlBasePath.empty()) {
        if (mtlBasePath.back() != std::filesystem::path::preferred_separator) {
            mtlBasePath += std::filesystem::path::preferred_separator;
        }
    }

    bool ok = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &error,
        path.c_str(),
        mtlBasePath.c_str()
    );
    if (!ok || !error.empty()) {
        throw std::runtime_error(fmt::format("Failed to load mesh {}: {}", name, error));
    }

    std::unordered_map<size_t, size_t> vertexHashIndexMap;
    std::vector<Vertex> newVertices;
    std::vector<Mesh::IndexType> newIndices;
    int materialIdx = -1;

    for (const auto &shape : shapes) {
        size_t indexOffset = 0;
        const auto &mesh = shape.mesh;

        for (size_t faceIndex = 0; faceIndex < mesh.num_face_vertices.size(); ++faceIndex) {
            if (mesh.num_face_vertices[faceIndex] != 3) {
                throw std::runtime_error(
                    fmt::format("Failed to load mesh {}: At least one face is not triangular", name)
                );
            }
            if (materialIdx == -1) {
                materialIdx = mesh.material_ids[faceIndex];
            }

            // 3 vertices per face for a triangle, inverting winding order
            std::array<size_t, 3> triangleIndices = {0, 2, 1};
            for (size_t faceVertexIndex : triangleIndices) {
                const auto &index = mesh.indices[indexOffset + faceVertexIndex];

                float x = attrib.vertices[3 * index.vertex_index];
                float y = attrib.vertices[3 * index.vertex_index + 1];
                float z = attrib.vertices[3 * index.vertex_index + 2];

                float nx = 0.f;
                float ny = 0.f;
                float nz = 0.f;

                float u = 0.f;
                float v = 0.f;

                if (index.normal_index >= 0) {
                    nx = attrib.normals[3 * index.normal_index];
                    ny = attrib.normals[3 * index.normal_index + 1];
                    nz = attrib.normals[3 * index.normal_index + 2];
                }

                if (index.texcoord_index >= 0) {
                    u = attrib.texcoords[2 * index.texcoord_index];
                    v = attrib.texcoords[2 * index.texcoord_index + 1];
                }

                Vertex vertex{
                    .position{x, y, z},
                    .normal{nx, ny, nz},
                    .color{1.f},
                    .uv{u, v},
                };

                // push vertex avoiding duplicates
                std::hash<Vertex> hasher;
                size_t hash = hasher(vertex);
                auto vertexHashIndexIter = vertexHashIndexMap.find(hash);
                if (vertexHashIndexIter != vertexHashIndexMap.end()) {
                    newIndices.push_back(vertexHashIndexIter->second);
                }
                else {
                    size_t vi = newVertices.size();
                    newVertices.push_back(vertex);
                    newIndices.push_back(vi);
                    vertexHashIndexMap.insert(std::make_pair(hash, vi));
                }
            }
            indexOffset += 3;
        }
    }


    std::map<int, const MaterialResource *> materialResources;
    for (int i = 0; i < static_cast<int>(materials.size()); ++i) {
        materialResources[i] = loadObjMaterial(materials[i]);
    }
    // TODO: actually assign all materials and not just the first
    const auto iter = materialResources.find(materialIdx);
    const MaterialResource *mat =  iter != materialResources.end() ? iter->second : nullptr;
    insertMesh(
        name,
        Mesh{std::move(newVertices), std::move(newIndices), mat}
    );
}


const MaterialResource *ResourceRepository::loadObjMaterial(const tinyobj::material_t &material)
{
    const auto iter = materials.find(material.name);
    if (iter != materials.end()) {
        return &iter->second;
    }

    spdlog::info("Loading material {}, ambient texture {} ", material.name, material.ambient_texname);

    return &materials.emplace(material.name, MaterialResource{
        .ambient = glm::vec3(material.ambient[0], material.ambient[1], material.ambient[2]),
        .diffuse = glm::vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]),
        .specular = glm::vec3(material.specular[0], material.specular[1], material.specular[2]),
        .shininess = material.shininess,
        .ambientTexture = hasImage(material.ambient_texname) ? &getImage(material.ambient_texname) : nullptr,
        .diffuseTexture = hasImage(material.diffuse_texname) ? &getImage(material.diffuse_texname) : nullptr,
        .specularTexture = hasImage(material.specular_texname) ? &getImage(material.specular_texname) : nullptr,
        .normalTexture = hasImage(material.normal_texname) ? &getImage(material.normal_texname) : nullptr,
        .vertexShader = &getVertexShader("shader/shader.vert"),
        .fragmentShader = &getFragmentShader("shader/shader.frag"),
        .name{material.name},
    }).first->second;
}

void ResourceRepository::loadImage(const ResourceKey &name, const std::filesystem::path &path)
{
    spdlog::info("Loading image {} ", path.string());

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
        + materials.size()
        + images.size()
        + vertexShaders.size()
        + fragmentShaders.size();
    std::vector<std::string> keys;
    keys.reserve(count);

    for (const auto &i : meshes) {
        keys.push_back(fmt::format("{} ({})", i.first, (void *) &i.second));
    }
    for (const auto &i : materials) {
        keys.push_back(fmt::format("{} ({})", i.first, (void *) &i.second));
    }
    for (const auto &i : images) {
        keys.push_back(fmt::format("{} ({})", i.first, (void *) &i.second));
    }
    for (const auto &i : vertexShaders) {
        keys.push_back(fmt::format("{} ({})", i.first, (void *) &i.second));
    }
    for (const auto &i : fragmentShaders) {
        keys.push_back(fmt::format("{} ({})", i.first, (void *) &i.second));
    }

    std::sort(keys.begin(), keys.end());

    std::string output;
    std::string indentationPrefix = std::string(indentationLevel, '\t');
    for (const auto &k : keys) {
        output += indentationPrefix + k + "\n";
    }

    return output;
}

void ResourceRepository::load(const path &path, const std::string extension)
{
    std::string resourceName = path.lexically_relative(current_path()).generic_string();
    resourceName = resourceName.substr(0, resourceName.size() - extension.size());

    try {
        if (extension == ".obj") {
            loadObj(resourceName, path);
        }
        else if (extension == ".png") {
            loadImage(resourceName, path);
        }
        else if(extension == ".spv") {
            if (resourceName.rfind(".frag") != std::string::npos) {
                loadFragmentShader(resourceName, path);
            }
            else if (resourceName.rfind(".vert") != std::string::npos) {
                loadVertexShader(resourceName, path);
            }
        }
        else {
            spdlog::warn("No loader for resource {}{}", resourceName, extension);
        }
    }
    catch (std::exception &e) {
        spdlog::error("ResourceRepository: Loading resource {} failed: {}!", resourceName, e.what());
    }
}

void ResourceRepository::loadAll()
{
    std::array<std::string, 2> loadLastExtensions = {
        ".obj",
    };
    std::unordered_map<std::string, std::vector<path>> resourcePaths;
    std::unordered_map<std::string, std::vector<path>> loadLastResourcePaths;

    auto currentPath = current_path();
    recursive_directory_iterator iterator(currentPath);

    for (auto &i : iterator) {
        if (i.is_regular_file()) {
            std::string extension = i.path().extension();
            if (std::find(loadLastExtensions.begin(), loadLastExtensions.end(), extension)
                != loadLastExtensions.end()
            ) {
                loadLastResourcePaths[extension].push_back(i.path());
            }
            else {
                resourcePaths[extension].push_back(i.path());
            }
        }
    }

    for (const auto &extAndPaths : resourcePaths) {
        for (const auto &path : extAndPaths.second) {
            load(path, extAndPaths.first);
        }
    }

    for (const auto &extAndPaths : loadLastResourcePaths) {
        for (const auto &path : extAndPaths.second) {
            load(path, extAndPaths.first);
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
