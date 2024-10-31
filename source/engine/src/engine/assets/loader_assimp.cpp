#include "loader_assimp.h"

#include <assimp/pbrmaterial.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>

namespace my {

bool LoaderAssimp::Load(Scene* p_data) {
    m_scene = p_data;
    Assimp::Importer importer;

    unsigned int flag = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_FlipUVs;

    const aiScene* aiscene = importer.ReadFile(m_filePath, flag);

    // check for errors
    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode)  // if is Not Zero
    {
        m_error = std::format("Error: failed to import scene '{}'\n\tdetails: {}", m_filePath, importer.GetErrorString());
        return false;
    }

    const uint32_t numMeshes = aiscene->mNumMeshes;
    const uint32_t numMaterials = aiscene->mNumMaterials;

    // LOG_VERBOSE("scene '{}' has {} meshes, {} materials", m_file_path, numMeshes, numMaterials);

    for (uint32_t i = 0; i < numMaterials; ++i) {
        ProcessMaterial(*aiscene->mMaterials[i]);
    }

    for (uint32_t i = 0; i < numMeshes; ++i) {
        ProcessMesh(*aiscene->mMeshes[i]);
    }

    ecs::Entity root = ProcessNode(aiscene->mRootNode, ecs::Entity::INVALID);
    m_scene->GetComponent<NameComponent>(root)->SetName(m_fileName);

    m_scene->m_root = root;
    return true;
}

void LoaderAssimp::ProcessMaterial(aiMaterial& p_material) {
    auto material_id = m_scene->CreateMaterialEntity(std::string("Material::") + p_material.GetName().C_Str());
    MaterialComponent* materialComponent = m_scene->GetComponent<MaterialComponent>(material_id);
    DEV_ASSERT(materialComponent);

    auto getMaterialPath = [&](aiTextureType p_type, uint32_t p_index) -> std::string {
        aiString path;
        if (p_material.GetTexture(p_type, p_index, &path) == AI_SUCCESS) {
            return m_basePath + path.C_Str();
        }
        return "";
    };

    std::string path = getMaterialPath(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE);
    if (path.empty()) {
        path = getMaterialPath(aiTextureType_DIFFUSE, 0);
    }
    materialComponent->requestImage(MaterialComponent::TEXTURE_BASE, path);

    path = getMaterialPath(aiTextureType_NORMALS, 0);
    if (path.empty()) {
        path = getMaterialPath(aiTextureType_HEIGHT, 0);
    }

    materialComponent->requestImage(MaterialComponent::TEXTURE_NORMAL, path);

    path = getMaterialPath(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE);
    materialComponent->requestImage(MaterialComponent::TEXTURE_METALLIC_ROUGHNESS, path);

    m_materials.emplace_back(material_id);
}

void LoaderAssimp::ProcessMesh(const aiMesh& p_mesh) {
    DEV_ASSERT(p_mesh.mNumVertices);
    const std::string mesh_name(p_mesh.mName.C_Str());
    const bool has_uv = p_mesh.mTextureCoords[0];
    if (!has_uv) {
        LOG_WARN("mesh {} does not have texture coordinates", mesh_name);
    }

    ecs::Entity mesh_id = m_scene->CreateMeshEntity("Mesh::" + mesh_name);
    MeshComponent& mesh_component = *m_scene->GetComponent<MeshComponent>(mesh_id);

    for (uint32_t i = 0; i < p_mesh.mNumVertices; ++i) {
        auto& position = p_mesh.mVertices[i];
        mesh_component.positions.emplace_back(vec3(position.x, position.y, position.z));
        auto& normal = p_mesh.mNormals[i];
        mesh_component.normals.emplace_back(vec3(normal.x, normal.y, normal.z));
        if (p_mesh.mTangents) {
            auto& tangent = p_mesh.mTangents[i];
            mesh_component.tangents.emplace_back(vec3(tangent.x, tangent.y, tangent.z));
        }

        if (has_uv) {
            auto& uv = p_mesh.mTextureCoords[0][i];
            mesh_component.texcoords_0.emplace_back(vec2(uv.x, uv.y));
        } else {
            mesh_component.texcoords_1.emplace_back(vec2(0));
        }
    }

    for (uint32_t i = 0; i < p_mesh.mNumFaces; ++i) {
        aiFace& face = p_mesh.mFaces[i];
        mesh_component.indices.emplace_back(face.mIndices[0]);
        mesh_component.indices.emplace_back(face.mIndices[1]);
        mesh_component.indices.emplace_back(face.mIndices[2]);
    }

    DEV_ASSERT(m_materials.size());
    MeshComponent::MeshSubset subset;
    subset.index_count = (uint32_t)mesh_component.indices.size();
    subset.index_offset = 0;
    subset.material_id = m_materials.at(p_mesh.mMaterialIndex);
    mesh_component.subsets.emplace_back(subset);

    mesh_component.createRenderData();

    m_meshes.push_back(mesh_id);
}

ecs::Entity LoaderAssimp::ProcessNode(const aiNode* p_node, ecs::Entity p_parent) {
    const auto key = std::string(p_node->mName.C_Str());

    ecs::Entity entity;

    if (p_node->mNumMeshes == 1) {  // geometry node
        entity = m_scene->CreateObjectEntity("Geometry::" + key);

        ObjectComponent& objComponent = *m_scene->GetComponent<ObjectComponent>(entity);
        objComponent.meshId = m_meshes[p_node->mMeshes[0]];
    } else {  // else make it a transform/bone node
        entity = m_scene->CreateTransformEntity("Node::" + key);

        for (uint32_t i = 0; i < p_node->mNumMeshes; ++i) {
            ecs::Entity child = m_scene->CreateObjectEntity("");
            auto tagComponent = m_scene->GetComponent<NameComponent>(child);
            tagComponent->SetName("SubGeometry_" + std::to_string(child.GetId()));
            ObjectComponent& objComponent = *m_scene->GetComponent<ObjectComponent>(child);
            objComponent.meshId = m_meshes[p_node->mMeshes[i]];
            m_scene->AttachComponent(child, entity);
        }
    }

    DEV_ASSERT(m_scene->Contains<TransformComponent>(entity));

    const aiMatrix4x4& local = p_node->mTransformation;                     // row major matrix
    mat4 localTransformColumnMajor(local.a1, local.b1, local.c1, local.d1,  // x0 y0 z0 w0
                                   local.a2, local.b2, local.c2, local.d2,  // x1 y1 z1 w1
                                   local.a3, local.b3, local.c3, local.d3,  // x2 y2 z2 w2
                                   local.a4, local.b4, local.c4, local.d4   // x3 y3 z3 w3
    );
    TransformComponent& transform = *m_scene->GetComponent<TransformComponent>(entity);
    transform.MatrixTransform(localTransformColumnMajor);

    if (p_parent.IsValid()) {
        m_scene->AttachComponent(entity, p_parent);
    }

    // process children
    for (uint32_t childIndex = 0; childIndex < p_node->mNumChildren; ++childIndex) {
        ProcessNode(p_node->mChildren[childIndex], entity);
    }

    return entity;
}

}  // namespace my
