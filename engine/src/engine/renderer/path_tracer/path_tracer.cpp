#include "path_tracer.h"

#include <algorithm>

#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/renderer/bvh_accel.h"
#include "engine/scene/scene.h"

// @TODO
#include "engine/math/detail/matrix.h"

namespace my {

void ConstructScene(const Scene& p_scene, GpuScene& p_gpu_scene) {
    for (auto [id, mesh] : p_scene.m_MeshComponents) {
        if (!mesh.bvh) {
            mesh.bvh = BvhAccel::Construct(mesh.indices, mesh.positions);
        }

        mesh.bvh->FillGpuBvhAccel(0, p_gpu_scene.bvhs);
        p_gpu_scene.vertices.reserve(mesh.positions.size());
        p_gpu_scene.triangles.reserve(mesh.indices.size() / 3);

        for (const auto& position : mesh.positions) {
            p_gpu_scene.vertices.push_back({ position });
        }
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            p_gpu_scene.triangles.emplace_back(Vector3i(mesh.indices[i],
                                                        mesh.indices[i + 1],
                                                        mesh.indices[i + 2]));
        }

        // @HACK: only supports one mesh
        if (id.IsValid()) {
            break;
        }
    }

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
}

}  // namespace my
