#include "path_tracer.h"

#include <algorithm>

#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/os/timer.h"
#include "engine/renderer/path_tracer/bvh_accel.h"
#include "engine/scene/scene.h"

// @TODO: remove
#include "engine/math/detail/matrix.h"

// @TODO: refactor
#include "engine/core/framework/graphics_manager.h"

namespace my {
#include "shader_resource_defines.hlsl.h"
}  // namespace my

namespace my {

template<typename T>
static auto CreateBuffer(GraphicsManager* p_gm, uint32_t p_slot, const std::vector<T>& p_data) {
    GpuBufferDesc desc{
        .slot = p_slot,
        .elementSize = sizeof(T),
        .elementCount = static_cast<uint32_t>(p_data.size()),
        .initialData = p_data.data(),
    };

    return p_gm->CreateStructuredBuffer(desc);
}

static void ConstructMesh(const MeshComponent& p_mesh, GpuScene& p_gpu_scene) {
    if (!p_mesh.bvh) {
        p_mesh.bvh = BvhAccel::Construct(p_mesh.indices, p_mesh.positions);
    }

    p_mesh.bvh->FillGpuBvhAccel(p_gpu_scene.bvhs);
    for (size_t i = 0; i < p_mesh.positions.size(); ++i) {
        GpuPtVertex vertex;
        vertex.position = p_mesh.positions[i];
        vertex.normal = p_mesh.normals[i];
        p_gpu_scene.vertices.emplace_back(vertex);
    }

    for (size_t i = 0; i < p_mesh.indices.size(); i += 3) {
        GpuPtIndex index;
        index.tri = Vector3i(p_mesh.indices[i],
                             p_mesh.indices[i + 1],
                             p_mesh.indices[i + 2]);
        p_gpu_scene.indices.emplace_back(index);
    }
}

void PathTracer::Update(const Scene& p_scene) {
    switch (m_mode) {
        case PathTracerMode::NONE:
            return;
        case PathTracerMode::TILED:
            LOG_ERROR("Not implmeneted");
            [[fallthrough]];
        case PathTracerMode::INTERACTIVE:
            break;
        default:
            CRASH_NOW_MSG("Invalid mode");
            return;
    }

    if (!m_ptIndexBuffer) {
        CreateAccelStructure(p_scene);
    }

    std::vector<GpuPtMesh> meshes;
    UpdateAccelStructure(p_scene);
    // @TODO: update mesh buffers
}

static void AppendVertices(const std::vector<GpuPtVertex>& p_source, std::vector<GpuPtVertex>& p_dest) {
#if USING(ENABLE_ASSERT)
    const int offset = (int)p_dest.size();
    const int count = (int)p_source.size();
#endif

    p_dest.insert(p_dest.end(), p_source.begin(), p_source.end());

    DEV_ASSERT((int)p_dest.size() == offset + count);
}

static void AppendIndices(const std::vector<GpuPtIndex>& p_source, std::vector<GpuPtIndex>& p_dest, int p_vertex_count) {
    const int offset = (int)p_dest.size();
    const int count = (int)p_source.size();
    p_dest.resize(offset + count);
    for (int i = 0; i < count; ++i) {
        const auto& source = p_source[i];
        auto& dest = p_dest[i + offset];
        dest.tri = source.tri + p_vertex_count;
    }
}

static void AppendBvhs(const std::vector<GpuPtBvh>& p_source, std::vector<GpuPtBvh>& p_dest, int p_index_offset) {
    const int offset = (int)p_dest.size();
    auto adjust_index = [offset](int& p_index) {
        if (p_index < 0) {
            return;
        }

        p_index += offset;
    };

    const int count = (int)p_source.size();
    p_dest.resize(offset + count);

    for (int i = 0; i < count; ++i) {
        const auto& source = p_source[i];
        auto& dest = p_dest[i + offset];
        dest = source;

        adjust_index(dest.hitIdx);
        adjust_index(dest.missIdx);
        if (dest.triangleIndex >= 0) {
            dest.triangleIndex += p_index_offset;
        }
    }
}

void PathTracer::UpdateAccelStructure(const Scene& p_scene) {
    const auto dirty_flag = p_scene.GetDirtyFlags();
    auto gm = GraphicsManager::GetSingletonPtr();

    std::map<ecs::Entity, int> materials_lookup;
    {
        std::vector<GpuPtMaterial> materials;
        for (auto mesh_it : m_meshs) {
            auto material_id = mesh_it.second.materialId;
            auto mat_it = materials_lookup.find(material_id);
            if (mat_it != materials_lookup.end()) {
                continue;
            }

            const auto material = p_scene.GetComponent<MaterialComponent>(material_id);
            DEV_ASSERT(material);

            materials_lookup[material_id] = (int)materials.size();

            GpuPtMaterial gpu_mat;
            gpu_mat.baseColor = material->baseColor.xyz;
            gpu_mat.emissive = gpu_mat.baseColor * material->emissive;
            gpu_mat.roughness = material->roughness;
            gpu_mat.metallic = material->metallic;
            materials.emplace_back(gpu_mat);
        }

        GpuBufferDesc desc{
            .slot = GetGlobalPtMeshesSlot(),
            .elementSize = sizeof(materials[0]),
            .elementCount = static_cast<uint32_t>(materials.size()),
            .initialData = materials.data(),
        };

        // @TODO: only change when material is dirty
        if ((dirty_flag != SCENE_DIRTY_NONE) || m_ptMaterialBuffer == nullptr) {
            m_ptMaterialBuffer = *gm->CreateStructuredBuffer(desc);
        }
    }

    {
        const auto view = p_scene.View<ObjectComponent>();

        std::vector<GpuPtMesh> meshes;
        meshes.reserve(view.GetSize());
        for (auto [id, object] : view) {
            auto transform = p_scene.GetComponent<TransformComponent>(id);
            auto mesh = p_scene.GetComponent<MeshComponent>(object.meshId);
            if (DEV_VERIFY(transform && mesh)) {
                auto mesh_it = m_meshs.find(object.meshId);
                if (mesh_it == m_meshs.end()) {
                    CRASH_NOW_MSG("mesh not found");
                    continue;
                }

                GpuPtMesh gpu_pt_mesh;
                gpu_pt_mesh.transform = transform->GetWorldMatrix();
                gpu_pt_mesh.transformInv = glm::inverse(gpu_pt_mesh.transform);
                const auto& cache = mesh_it->second;
                gpu_pt_mesh.rootBvhId = cache.rootBvhId;
                auto mat_it = materials_lookup.find(cache.materialId);
                DEV_ASSERT(mat_it != materials_lookup.end());
                gpu_pt_mesh.materialId = mat_it->second;

                // @TODO: fill offsets
                meshes.push_back(gpu_pt_mesh);
            }
        }

        GpuBufferDesc desc{
            .slot = GetGlobalPtMeshesSlot(),
            .elementSize = sizeof(meshes[0]),
            .elementCount = static_cast<uint32_t>(meshes.size()),
            .initialData = meshes.data(),
        };

        if ((dirty_flag & SCENE_DIRTY_WORLD) || m_ptMeshBuffer == nullptr) {
            m_ptMeshBuffer = *gm->CreateStructuredBuffer(desc);
        }
    }
}

bool PathTracer::CreateAccelStructure(const Scene& p_scene) {
    DEV_ASSERT(m_ptVertexBuffer == nullptr);

    auto gm = GraphicsManager::GetSingletonPtr();

    Timer timer;
    GpuScene gpu_scene;

    // meshes
    for (auto [id, object] : p_scene.m_ObjectComponents) {
        auto transform = p_scene.GetComponent<TransformComponent>(id);
        auto mesh = p_scene.GetComponent<MeshComponent>(object.meshId);
        if (DEV_VERIFY(transform && mesh)) {
            auto it = m_meshs.find(object.meshId);
            if (it != m_meshs.end()) {
                continue;
            }

            const int bvh_count = (int)gpu_scene.bvhs.size();
            const int index_count = (int)gpu_scene.indices.size();
            const int vertex_count = (int)gpu_scene.vertices.size();

            MeshData meta{
                .rootBvhId = bvh_count,
                .materialId = mesh->subsets[0].material_id
            };
            m_meshs[object.meshId] = meta;

            GpuScene tmp_scene;
            ConstructMesh(*mesh, tmp_scene);

            AppendVertices(tmp_scene.vertices, gpu_scene.vertices);
            AppendIndices(tmp_scene.indices, gpu_scene.indices, vertex_count);
            AppendBvhs(tmp_scene.bvhs, gpu_scene.bvhs, index_count);
        }
    }

    const uint32_t triangle_count = (uint32_t)gpu_scene.indices.size();
    const uint32_t bvh_count = (uint32_t)gpu_scene.bvhs.size();

    m_ptBvhBuffer = *CreateBuffer(gm, GetGlobalPtBvhsSlot(), gpu_scene.bvhs);
    m_ptVertexBuffer = *CreateBuffer(gm, GetGlobalPtVerticesSlot(), gpu_scene.vertices);
    m_ptIndexBuffer = *CreateBuffer(gm, GetGlobalPtIndicesSlot(), gpu_scene.indices);

    LOG("Path tracer scene loaded in {}, contains {} triangles, {} BVH",
        timer.GetDurationString(),
        triangle_count,
        bvh_count);

    /// materials
#if 0
    for (auto [entity, material] : p_scene.m_MaterialComponents) {

        auto fill_texture = [&](int p_index, int& p_out_enabled, sampler2D& p_out_handle) {
            const ImageAsset* image = AssetRegistry::GetSingleton().GetAssetByHandle<ImageAsset>(material.textures[p_index].path);
            if (image && image->gpu_texture) {
                uint64_t handle = image->gpu_texture->GetResidentHandle();
                if (handle) {
                    p_out_enabled = true;
                    if (is_opengl) {
                        p_out_handle.Set64(handle);

                    } else {
                        p_out_handle.Set32(static_cast<int>(handle));
                    }
                    return true;
                }
            }
            return false;
        };

        fill_texture(MaterialComponent::TEXTURE_BASE, gpu_mat.has_base_color_map, gpu_mat.base_color_map_handle);
        fill_texture(MaterialComponent::TEXTURE_NORMAL, gpu_mat.has_normal_map, gpu_mat.normal_map_handle);
        fill_texture(MaterialComponent::TEXTURE_METALLIC_ROUGHNESS, gpu_mat.has_material_map, gpu_mat.material_map_handle);

        p_out_scene.materials.push_back(gpu_mat);
        material_lut[entity] = static_cast<int>(p_out_scene.materials.size()) - 1;
    }

    /// objects
    p_out_scene.geometries.clear();
    for (auto [entity, light] : p_scene.m_LightComponents) {
        if (light.GetType() == LIGHT_TYPE_INFINITE) {
            auto transform = p_scene.GetComponent<TransformComponent>(entity);
            DEV_ASSERT(transform);
            Vector3f rotation = (transform->GetWorldMatrix() * Vector4f::UnitZ).xyz;
            rotation = normalize(rotation);
            float radius = 1000.0f;

            Vector3f tmp;
            tmp.Set(&rotation.x);
            Vector3f center = radius * 0.5f * tmp;

            auto it = material_lut.find(entity);
            DEV_ASSERT(it != material_lut.end());
            int material_id = it->second;
            gpu_geometry_t sphere(center, radius, material_id);
            // HACK: increase emissive power
            p_out_scene.materials[material_id].emissive *= 5.0f;
            // gpu_geometry_t sphere(center, radius, it->second);
            tmp_gpu_objects.push_back(sphere);
        }
    }
#endif

    return true;
}

bool PathTracer::IsActive() const {
    if (m_mode != PathTracerMode::INTERACTIVE) {
        return false;
    }

    if (m_ptBvhBuffer == nullptr) {
        return false;
    }

    return true;
}

void PathTracer::BindData(GraphicsManager& p_gm) {
    // @TODO: check null
    p_gm.BindStructuredBuffer(GetGlobalPtMeshesSlot(), m_ptMeshBuffer.get());
    p_gm.BindStructuredBuffer(GetGlobalPtBvhsSlot(), m_ptBvhBuffer.get());
    p_gm.BindStructuredBuffer(GetGlobalPtVerticesSlot(), m_ptVertexBuffer.get());
    p_gm.BindStructuredBuffer(GetGlobalPtIndicesSlot(), m_ptIndexBuffer.get());
    p_gm.BindStructuredBuffer(GetGlobalPtMaterialsSlot(), m_ptMaterialBuffer.get());
}

void PathTracer::UnbindData(GraphicsManager& p_gm) {
    p_gm.UnbindStructuredBuffer(GetGlobalPtBvhsSlot());
    p_gm.UnbindStructuredBuffer(GetGlobalPtVerticesSlot());
    p_gm.UnbindStructuredBuffer(GetGlobalPtIndicesSlot());
    p_gm.UnbindStructuredBuffer(GetGlobalPtMeshesSlot());
    p_gm.UnbindStructuredBuffer(GetGlobalPtMaterialsSlot());
}

}  // namespace my
