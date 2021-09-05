#pragma once
#include "VulkanDevice.hpp"
#include "Renderable.hpp"
#include "TextureManager.hpp"
#include <assimp/scene.h>
#include <assimp/mesh.h>


struct aiMesh;
struct aiScene;

namespace render
{

class AssetLoader
{
public:
    AssetLoader(std::shared_ptr<VulkanDevice>, std::shared_ptr<memory::TextureManager>);
    std::shared_ptr<Renderable> loadObject(const std::string& path, std::shared_ptr<Pipeline>);

private:
    Mesh processMesh(
        const struct aiMesh* mesh,
        const struct aiScene* scene,
        const std::string& path_root);

    void processNodes(
        std::vector<Mesh>& meshes,
        const struct aiNode* node,
        const struct aiScene* scene,
        const std::string& dir_root);

    uint32_t loadTexture(
        const struct aiMaterial* material,
        enum aiTextureType type,
        const std::string& path_root);

    std::shared_ptr<VulkanDevice> device;
    std::shared_ptr<memory::TextureManager> tex_mgr;
};

} // namespace render
