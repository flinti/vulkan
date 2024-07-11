#ifndef RESOURCEREPOSITORY_H_
#define RESOURCEREPOSITORY_H_

#include "Mesh.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

typedef std::string ResourceKey;

struct ImageResource
{
    uint32_t width;
    uint32_t height;
    void *data;
};

typedef std::vector<std::byte> ShaderResource;

class ResourceRepository
{
public:
    ResourceRepository();
    ResourceRepository(const ResourceRepository &) = delete;
    ResourceRepository(ResourceRepository &&) = default;
    ~ResourceRepository();

    const Mesh &getMesh(const ResourceKey &name) const;
    const ImageResource &getImage(const ResourceKey &name) const;
    const ShaderResource &getFragmentShader(const ResourceKey &name) const;
    const ShaderResource &getVertexShader(const ResourceKey &name) const;

    bool insertMesh(const ResourceKey &name, Mesh mesh);

    void loadImage(const ResourceKey &name, const std::filesystem::path &path);
    void loadFragmentShader(const ResourceKey &name, const std::filesystem::path &path);
    void loadVertexShader(const ResourceKey &name, const std::filesystem::path &path);

    std::string resourceTree(size_t indentationLevel = 0) const;
private:
    void load(const std::filesystem::directory_entry &dirEntry);
    void loadAll();

    std::vector<std::byte> readShaderFile(const std::filesystem::path &path);

    std::unordered_map<ResourceKey, Mesh> meshes;
    std::unordered_map<ResourceKey, ImageResource> images;
    std::unordered_map<ResourceKey, ShaderResource> vertexShaders;
    std::unordered_map<ResourceKey, ShaderResource> fragmentShaders;
};

#endif
