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
        p_gpu_scene.vertices.push_back({ p_mesh.positions[i], p_mesh.normals[i] });
    }

    for (size_t i = 0; i < p_mesh.indices.size(); i += 3) {
        p_gpu_scene.indices.emplace_back(Vector3i(p_mesh.indices[i],
                                                  p_mesh.indices[i + 1],
                                                  p_mesh.indices[i + 2]));
    }
}

void PathTracer::UpdateAccelStructure(const Scene& p_scene) {
    const auto view = p_scene.View<ObjectComponent>();

    std::vector<GpuPtMesh> meshes;
    meshes.reserve(view.GetSize());
    for (auto [id, object] : view) {
        auto transform = p_scene.GetComponent<TransformComponent>(id);
        auto mesh = p_scene.GetComponent<MeshComponent>(object.meshId);
        if (DEV_VERIFY(transform && mesh)) {
            auto it = m_lut.find(object.meshId);
            if (it == m_lut.end()) {
                LOG_WARN("mesh not found");
                continue;
            }

            GpuPtMesh gpu_pt_mesh;
            gpu_pt_mesh.transform = transform->GetWorldMatrix();
            gpu_pt_mesh.transformInv = glm::inverse(gpu_pt_mesh.transform);
            gpu_pt_mesh.rootBvhId = it->second.rootBvhId;

            // @TODO: fill offsets
            meshes.push_back(gpu_pt_mesh);
        }
    }

    auto gm = GraphicsManager::GetSingletonPtr();

    GpuBufferDesc desc{
        .slot = GetGlobalPtMeshesSlot(),
        .elementSize = sizeof(meshes[0]),
        .elementCount = static_cast<uint32_t>(meshes.size()),
        .initialData = meshes.data(),
    };

    // @TODO: only update when dirty performance
    m_ptMeshBuffer = *gm->CreateStructuredBuffer(desc);
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
    p_dest.insert(p_dest.end(), p_source.begin(), p_source.end());
}

static void AppendIndices(const std::vector<Vector3i>& p_source, std::vector<Vector3i>& p_dest, int p_vertex_count) {
    const int offset = (int)p_dest.size();
    const int count = (int)p_source.size();
    p_dest.resize(offset + count);
    for (int i = 0; i < count; ++i) {
        const auto& source = p_source[i];
        auto& dest = p_dest[i + offset];
        dest = source + p_vertex_count;
    }
}

static void AppendBvhs(const std::vector<GpuPtBvh>& p_source, std::vector<GpuPtBvh>& p_dest, int p_index_offset) {
    const int offset = (int)p_dest.size();
    const int count = (int)p_source.size();
    p_dest.resize(offset + count);

    auto adjust_index = [offset](int& p_index) {
        if (p_index == -1) {
            return;
        }

        p_index += offset;
    };

    for (int i = 0; i < count; ++i) {
        const auto& source = p_source[i];
        auto& dest = p_dest[i + offset];
        dest = source;

        adjust_index(dest.hitIdx);
        adjust_index(dest.missIdx);
        dest.triangleIndex += p_index_offset;
    }
}

bool PathTracer::CreateAccelStructure(const Scene& p_scene) {
    DEV_ASSERT(m_ptVertexBuffer == nullptr);

    auto gm = GraphicsManager::GetSingletonPtr();

    Timer timer;
    GpuScene gpu_scene;
    for (auto [id, mesh] : p_scene.m_MeshComponents) {
        const int bvh_count = (int)gpu_scene.bvhs.size();
        const int index_count = (int)gpu_scene.indices.size();
        const int vertex_count = (int)gpu_scene.vertices.size();

        auto it = m_lut.find(id);
        if (it == m_lut.end()) {
            BvhMeta meta{
                .rootBvhId = bvh_count
            };
            m_lut[id] = meta;

            GpuScene tmp_scene;
            ConstructMesh(mesh, tmp_scene);

            AppendVertices(tmp_scene.vertices, gpu_scene.vertices);
            AppendIndices(tmp_scene.indices, gpu_scene.indices, vertex_count);
            AppendBvhs(tmp_scene.bvhs, gpu_scene.bvhs, index_count);
        }
    }

    const uint32_t triangle_count = (uint32_t)gpu_scene.indices.size();
    const uint32_t bvh_count = (uint32_t)gpu_scene.bvhs.size();

    m_ptBvhBuffer = *CreateBuffer(gm, GetGlobalRtBvhsSlot(), gpu_scene.bvhs);
    m_ptVertexBuffer = *CreateBuffer(gm, GetGlobalRtVerticesSlot(), gpu_scene.vertices);
    m_ptIndexBuffer = *CreateBuffer(gm, GetGlobalRtIndicesSlot(), gpu_scene.indices);

    LOG("Path tracer scene loaded in {}, contains {} triangles, {} BVH",
        timer.GetDurationString(),
        triangle_count,
        bvh_count);

    /// materials
#if 0
    using ecs::Entity;
    std::map<Entity, int> material_lut;

    [[maybe_unused]] const bool is_opengl = GraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;

    p_out_scene.materials.clear();
    for (auto [entity, material] : p_scene.m_MaterialComponents) {
        gpu_material_t gpu_mat;
        gpu_mat.albedo = material.baseColor.xyz;
        gpu_mat.emissive = material.emissive * gpu_mat.albedo;
        gpu_mat.roughness = material.roughness;
        gpu_mat.reflect_chance = material.metallic;

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

    auto add_object = [&](const MeshComponent& p_mesh, const TransformComponent& p_transform) {
        // Loop over shapes
        Matrix4x4f transform = p_transform.GetWorldMatrix();

        for (size_t index = 0; index < p_mesh.indices.size(); index += 3) {
            const uint32_t index0 = p_mesh.indices[index];
            const uint32_t index1 = p_mesh.indices[index + 1];
            const uint32_t index2 = p_mesh.indices[index + 2];

            Vector4f points[3] = {
                transform * Vector4f(p_mesh.positions[index0], 1.0f),
                transform * Vector4f(p_mesh.positions[index1], 1.0f),
                transform * Vector4f(p_mesh.positions[index2], 1.0f),
            };

            Vector4f normals[3] = {
                transform * Vector4f(p_mesh.normals[index0], 0.0f),
                transform * Vector4f(p_mesh.normals[index1], 0.0f),
                transform * Vector4f(p_mesh.normals[index2], 0.0f),
            };

            Vector2f uvs[3] = {
                p_mesh.texcoords_0[index0],
                p_mesh.texcoords_0[index1],
                p_mesh.texcoords_0[index2],
            };

            // per-face material
            gpu_geometry_t triangle(Vector3f(points[0].xyz),
                                    Vector3f(points[1].xyz),
                                    Vector3f(points[2].xyz), 0);
            triangle.uv1 = uvs[0];
            triangle.uv2 = uvs[1];
            triangle.uv3 = uvs[2];
            triangle.normal1 = normalize(Vector3f(normals[0].xyz));
            triangle.normal2 = normalize(Vector3f(normals[1].xyz));
            triangle.normal3 = normalize(Vector3f(normals[2].xyz));
            triangle.kind = gpu_geometry_t::Kind::Triangle;
            auto it = material_lut.find(p_mesh.subsets[0].material_id);
            DEV_ASSERT(it != material_lut.end());
            triangle.material_id = it->second;
            tmp_gpu_objects.push_back(triangle);
        }
    };

    /// objects
    p_out_scene.geometries.clear();
    for (auto [entity, object] : p_scene.m_ObjectComponents) {
        // ObjectComponent
        auto transform = p_scene.GetComponent<TransformComponent>(entity);
        auto mesh = p_scene.GetComponent<MeshComponent>(object.meshId);
        DEV_ASSERT(transform);
        DEV_ASSERT(mesh);
        add_object(*mesh, *transform);
    }
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

    Timer timer;
    /// construct bvh
    Bvh root(tmp_gpu_objects);
    root.CreateGpuBvh(p_out_scene.bvhs, p_out_scene.geometries);
    LOG("[PathTracer] bvh created in {}", timer.GetDurationString());

    p_out_scene.bbox = root.GetBox();

    /// adjust bbox
    for (gpu_bvh_t& bvh : p_out_scene.bvhs) {
        for (int i = 0; i < 3; ++i) {
            if (glm::abs(bvh.max[i] - bvh.min[i]) < 0.01f) {
                bvh.min[i] -= Box3::BOX_MIN_SIZE;
                bvh.max[i] += Box3::BOX_MIN_SIZE;
            }
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
    p_gm.BindStructuredBuffer(GetGlobalRtBvhsSlot(), m_ptBvhBuffer.get());
    p_gm.BindStructuredBuffer(GetGlobalRtVerticesSlot(), m_ptVertexBuffer.get());
    p_gm.BindStructuredBuffer(GetGlobalRtIndicesSlot(), m_ptIndexBuffer.get());
}

void PathTracer::UnbindData(GraphicsManager& p_gm) {
    p_gm.UnbindStructuredBuffer(GetGlobalRtBvhsSlot());
    p_gm.UnbindStructuredBuffer(GetGlobalRtVerticesSlot());
    p_gm.UnbindStructuredBuffer(GetGlobalRtIndicesSlot());
    p_gm.UnbindStructuredBuffer(GetGlobalPtMeshesSlot());
}

}  // namespace my
