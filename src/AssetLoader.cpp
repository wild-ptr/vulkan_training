#include "AssetLoader.hpp"
#include "Vertex.hpp"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <future>

namespace render
{
uint32_t AssetLoader::loadTexture(
    const struct aiMaterial* material,
    enum aiTextureType type,
    const std::string& path_root)
{
    struct aiString str;
    aiGetMaterialTexture(material, type, 0, &str, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr); // nice call bro

    return tex_mgr->loadTexture(path_root + str.data);
}

Mesh AssetLoader::processMesh(
        const struct aiMesh* mesh,
        const struct aiScene* scene,
        const std::string& dir_root)
{
    assert(mesh->mNumVertices);

    std::vector<Vertex> vertices;
    vertices.reserve(mesh->mNumVertices);

    std::vector<uint32_t> indices;
    indices.reserve(mesh->mNumFaces * 3);

    for(size_t i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex v{};
        v.pos[0] = mesh->mVertices[i].x;
        v.pos[1] = mesh->mVertices[i].y;
        v.pos[2] = mesh->mVertices[i].z;

        v.surf_normals[0] = mesh->mNormals[i].x;
        v.surf_normals[1] = mesh->mNormals[i].y;
        v.surf_normals[2] = mesh->mNormals[i].z;

        v.tangents[0] = mesh->mTangents[i].x;
        v.tangents[1] = mesh->mTangents[i].y;
        v.tangents[2] = mesh->mTangents[i].z;

        if(mesh->mTextureCoords[0])
        {
            v.tex_coords[0] = mesh->mTextureCoords[0][i].x;
            v.tex_coords[1] = mesh->mTextureCoords[0][i].y;
        }

        vertices.emplace_back(std::move(v));
    }

    for(size_t i = 0; i < mesh->mNumFaces; ++i)
    {
        for(size_t j = 0; j < 3; ++j)
        {
            indices.emplace_back(mesh->mFaces[i].mIndices[j]);
        }
    }

    MeshPushConstantData meshPushData{};
    if(mesh->mMaterialIndex >= 0)
    {
        struct aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        auto diffuseIdx = std::async(std::launch::async, &AssetLoader::loadTexture, this, material, aiTextureType_DIFFUSE, dir_root);
        auto normalIdx = std::async(std::launch::async, &AssetLoader::loadTexture, this, material, aiTextureType_HEIGHT, dir_root);
        auto specularIdx = std::async(std::launch::async, &AssetLoader::loadTexture, this, material, aiTextureType_SPECULAR, dir_root);
        meshPushData.diffuse_texid = diffuseIdx.get();
        meshPushData.normal_texid = normalIdx.get();
        meshPushData.specular_texid = specularIdx.get();
    }

    return Mesh{device, vertices, indices, meshPushData};
}

AssetLoader::AssetLoader(std::shared_ptr<VulkanDevice> dev_ptr, std::shared_ptr<memory::TextureManager> tex_ptr)
    : device(std::move(dev_ptr))
    , tex_mgr(std::move(tex_ptr))
{
}

void AssetLoader::processNodes(
        std::vector<Mesh>& meshes,
        const struct aiNode* node,
        const struct aiScene* scene,
        const std::string& dir_root)
{
    for(size_t i = 0; i < node->mNumMeshes; ++i)
    {
        struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(processMesh(mesh, scene, dir_root));
    }

    // do the same processing for every child node there is.
    for(size_t i = 0; i < node->mNumChildren; ++i)
    {
        processNodes(meshes, node, scene, dir_root);
    }

    // at this point we have a full vector of meshes for the scene. Return.
}

std::shared_ptr<Renderable> AssetLoader::loadObject(const std::string& path, std::shared_ptr<Pipeline> pipeline)
{
    const struct aiScene* scene = aiImportFile(path.c_str(),
        aiProcess_CalcTangentSpace |
        //aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph);

    if(not scene or scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE or not scene->mRootNode)
    {
        dbgE << "Failed to load model: " << path << NEWL;
        return nullptr;
    }

    // ugh
    auto last_slash = std::find(std::crbegin(path), std::crend(path), '/');
    size_t size = &*last_slash - &path[0]; // fuck reverse iterator incompatibilities, just go raw.
    std::string object_folder{&path[0], size};
    object_folder += '/';

    std::vector<Mesh> meshes;
    processNodes(meshes, scene->mRootNode, scene, object_folder);

    return std::make_shared<Renderable>(device, pipeline, meshes);
}

} // namespace render
