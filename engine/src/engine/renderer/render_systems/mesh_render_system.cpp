#include "engine/math/frustum.h"
#include "engine/math/geometry.h"
#include "engine/math/matrix_transform.h"
#include "engine/renderer/frame_data.h"
#include "engine/runtime/asset_registry.h"
#include "engine/scene/scene.h"

namespace my {

using FilterObjectFunc1 = std::function<bool(const ObjectComponent& p_object)>;
using FilterObjectFunc2 = std::function<bool(const AABB& p_object_aabb)>;

// @TODO: fix this function OMG
static void FillMaterialConstantBuffer(bool p_is_opengl, const MaterialComponent* p_material, MaterialConstantBuffer& cb) {
    cb.c_baseColor = p_material->baseColor;
    cb.c_metallic = p_material->metallic;
    cb.c_roughness = p_material->roughness;
    cb.c_emissivePower = p_material->emissive;

    auto set_texture = [&](int p_idx,
                           TextureHandle& p_out_handle,
                           sampler_t& p_out_resident_handle) {
        p_out_handle = 0;
        p_out_resident_handle.Set64(0);

        if (!p_material->textures[p_idx].enabled) {
            return false;
        }

        // @TODO: at least fix this
        const ImageAsset* image = AssetRegistry::GetSingleton().GetAssetByHandle<ImageAsset>(p_material->textures[p_idx].path);
        if (!image) {
            return false;
        }

        auto texture = reinterpret_cast<GpuTexture*>(image->gpu_texture.get());
        if (!texture) {
            return false;
        }

        const uint64_t resident_handle = texture->GetResidentHandle();

        p_out_handle = texture->GetHandle();
        if (p_is_opengl) {
            p_out_resident_handle.Set64(resident_handle);
        } else {
            p_out_resident_handle.Set32(static_cast<uint32_t>(resident_handle));
        }
        return true;
    };

    cb.c_hasBaseColorMap = set_texture(MaterialComponent::TEXTURE_BASE, cb.c_baseColorMapHandle, cb.c_BaseColorMapResidentHandle);
    cb.c_hasNormalMap = set_texture(MaterialComponent::TEXTURE_NORMAL, cb.c_normalMapHandle, cb.c_NormalMapResidentHandle);
    cb.c_hasMaterialMap = set_texture(MaterialComponent::TEXTURE_METALLIC_ROUGHNESS, cb.c_materialMapHandle, cb.c_MaterialMapResidentHandle);
};

// @TODO: refactor this
static void FillPass(const Scene& p_scene,
                     FilterObjectFunc1 p_filter1,
                     FilterObjectFunc2 p_filter2,
                     std::vector<RenderCommand>& p_commands,
                     FrameData& p_framedata) {

    for (auto [entity, obj] : p_scene.m_ObjectComponents) {
        if (!p_scene.Contains<TransformComponent>(entity)) {
            continue;
        }

        if (!p_filter1(obj)) {
            continue;
        }

        const TransformComponent& transform = *p_scene.GetComponent<TransformComponent>(entity);
        DEV_ASSERT(p_scene.Contains<MeshComponent>(obj.meshId));

        const MeshComponent& mesh = *p_scene.GetComponent<MeshComponent>(obj.meshId);

        const Matrix4x4f& world_matrix = transform.GetWorldMatrix();
        AABB aabb = mesh.localBound;
        aabb.ApplyMatrix(world_matrix);
        if (!p_filter2(aabb)) {
            continue;
        }

        PerBatchConstantBuffer batch_buffer;
        batch_buffer.c_worldMatrix = world_matrix;
        batch_buffer.c_meshFlag = mesh.armatureId.IsValid();

        DrawCommand draw;
        if (entity == p_scene.m_selected) {
            draw.flags = STENCIL_FLAG_SELECTED;
        }

        draw.batch_idx = p_framedata.batchCache.FindOrAdd(entity, batch_buffer);
        if (mesh.armatureId.IsValid()) {
            auto& armature = *p_scene.GetComponent<ArmatureComponent>(mesh.armatureId);
            DEV_ASSERT(armature.boneTransforms.size() <= MAX_BONE_COUNT);

            BoneConstantBuffer bone;
            memcpy(bone.c_bones, armature.boneTransforms.data(), sizeof(Matrix4x4f) * armature.boneTransforms.size());

            // @TODO: better memory usage
            draw.bone_idx = p_framedata.boneCache.FindOrAdd(mesh.armatureId, bone);
        } else {
            draw.bone_idx = -1;
        }

        draw.mesh_data = mesh.gpuResource.get();
        draw.mat_idx = -1;
        DEV_ASSERT(draw.mesh_data);

        draw.indexCount = static_cast<uint32_t>(mesh.indices.size());
        p_commands.emplace_back(RenderCommand::From(draw));
    }
}

static void FillLightBuffer(const Scene& p_scene, FrameData& p_framedata) {
    const uint32_t light_count = glm::min<uint32_t>((uint32_t)p_scene.GetCount<LightComponent>(), MAX_LIGHT_COUNT);

    auto& cache = p_framedata.perFrameCache;
    cache.c_lightCount = light_count;

    [[maybe_unused]] auto& point_shadow_cache = p_framedata.pointShadowCache;

    int idx = 0;
    for (auto [light_entity, light_component] : p_scene.View<LightComponent>()) {
        const TransformComponent* light_transform = p_scene.GetComponent<TransformComponent>(light_entity);
        const MaterialComponent* material = p_scene.GetComponent<MaterialComponent>(light_entity);

        DEV_ASSERT(light_transform && material);

        // SHOULD BE THIS INDEX
        Light& light = cache.c_lights[idx];
        bool cast_shadow = light_component.CastShadow();
        light.cast_shadow = cast_shadow;
        light.type = light_component.GetType();
        light.color = material->baseColor.xyz;
        light.color *= material->emissive;
        switch (light_component.GetType()) {
            case LIGHT_TYPE_INFINITE: {
                Matrix4x4f light_local_matrix = light_transform->GetLocalMatrix();
                Vector3f light_dir((light_local_matrix * Vector4f(0, 0, 1, 1)).xyz);
                light_dir = normalize(light_dir);
                cache.c_sunPosition = light_dir;
                light.cast_shadow = cast_shadow;
                light.position = light_dir;

                // @TODO: add option to specify extent
                // @would be nice if can add debug draw
                const AABB& world_bound = (light_component.HasShadowRegion()) ? light_component.m_shadowRegion : p_scene.GetBound();
                Vector3f center = world_bound.Center();
                Vector3f extents = world_bound.Size();

                const float size = 0.7f * max(extents.x, max(extents.y, extents.z));
                Vector3f tmp;
                tmp.Set(&light_dir.x);
                light.view_matrix = LookAtRh(center + tmp * size, center, Vector3f::UnitY);

                if (p_framedata.options.isOpengl) {
                    light.projection_matrix = BuildOpenGlOrthoRH(-size, size, -size, size, -size, 3.0f * size);
                } else {
                    light.projection_matrix = BuildOrthoRH(-size, size, -size, size, -size, 3.0f * size);
                }

                PerPassConstantBuffer pass_constant;
                // @TODO: Build correct matrices
                pass_constant.c_projectionMatrix = light.projection_matrix;
                pass_constant.c_viewMatrix = light.view_matrix;
                p_framedata.shadowPasses[0].pass_idx = static_cast<int>(p_framedata.passCache.size());
                p_framedata.passCache.emplace_back(pass_constant);

                // @TODO: fix
                Frustum light_frustum(light.projection_matrix * light.view_matrix);
                FillPass(
                    p_scene,
                    [](const ObjectComponent& p_object) {
                        return p_object.flags & ObjectComponent::FLAG_CAST_SHADOW;
                    },
                    [&](const AABB& p_aabb) {
                        return light_frustum.Intersects(p_aabb);
                    },
                    p_framedata.shadow_pass_commands,
                    p_framedata);
            } break;
            case LIGHT_TYPE_POINT: {
                [[maybe_unused]] const int shadow_map_index = light_component.GetShadowMapIndex();
                // @TODO: there's a bug in shadow map allocation
                light.atten_constant = light_component.m_atten.constant;
                light.atten_linear = light_component.m_atten.linear;
                light.atten_quadratic = light_component.m_atten.quadratic;
                light.position = light_component.GetPosition();
                light.cast_shadow = cast_shadow;
                light.max_distance = light_component.GetMaxDistance();
                LOG_WARN("TODO: fix light");
#if 0
                if (cast_shadow && shadow_map_index != -1) {
                    light.shadow_map_index = shadow_map_index;

                    Vector3f radiance(light.max_distance);
                    AABB aabb = AABB::FromCenterSize(light.position, radiance);
                    auto pass = std::make_unique<PassContext>();
                    FillPass(
                        p_scene,
                        *pass.get(),
                        [](const ObjectComponent& p_object) {
                            return p_object.flags & ObjectComponent::FLAG_CAST_SHADOW;
                        },
                        [&](const AABB& p_aabb) {
                            return p_aabb.Intersects(aabb);
                        },
                        p_out_data, false);

                    DEV_ASSERT_INDEX(shadow_map_index, MAX_POINT_LIGHT_SHADOW_COUNT);
                    const auto& light_matrices = light_component.GetMatrices();
                    for (int face_id = 0; face_id < 6; ++face_id) {
                        const uint32_t slot = shadow_map_index * 6 + face_id;
                        point_shadow_cache[slot].c_pointLightMatrix = light_matrices[face_id];
                        point_shadow_cache[slot].c_pointLightPosition = light_component.GetPosition();
                        point_shadow_cache[slot].c_pointLightFar = light_component.GetMaxDistance();
                    }

                    p_out_data.pointShadowPasses[shadow_map_index] = std::move(pass);
                } else {
                    light.shadow_map_index = -1;
                }
#endif
            } break;
            case LIGHT_TYPE_AREA: {
                Matrix4x4f transform = light_transform->GetWorldMatrix();
                constexpr float s = 0.5f;
                light.points[0] = transform * Vector4f(-s, +s, 0.0f, 1.0f);
                light.points[1] = transform * Vector4f(-s, -s, 0.0f, 1.0f);
                light.points[2] = transform * Vector4f(+s, -s, 0.0f, 1.0f);
                light.points[3] = transform * Vector4f(+s, +s, 0.0f, 1.0f);
            } break;
            default:
                CRASH_NOW();
                break;
        }
        ++idx;
    }
}

static void AddDebugCube(FrameData& p_framedata,
                         const AABB& p_aabb,
                         const Color& p_color,
                         const Matrix4x4f* p_transform = nullptr) {

    const auto& min = p_aabb.GetMin();
    const auto& max = p_aabb.GetMax();

    std::vector<Vector3f> positions;
    std::vector<uint32_t> indices;
    BoxWireFrameHelper(min, max, positions, indices);

    auto& context = p_framedata.drawDebugContext;
    for (const auto& i : indices) {
        const Vector3f& pos = positions[i];
        if (p_transform) {
            const auto tmp = *p_transform * Vector4f(pos, 1.0f);
            context.positions.emplace_back(Vector3f(tmp.xyz));
        } else {
            context.positions.emplace_back(Vector3f(pos));
        }
        context.colors.emplace_back(p_color);
    }
}

static void FillVoxelPass(const Scene& p_scene, FrameData& p_framedata) {
    bool enabled = false;
    bool show_debug = false;
    p_framedata.voxel_gi_bound.MakeInvalid();
    int counter = 0;
    for (auto [entity, voxel_gi] : p_scene.m_VoxelGiComponents) {
        p_framedata.voxel_gi_bound = voxel_gi.region;
        if (!p_framedata.voxel_gi_bound.IsValid()) {
            return;
        }

        show_debug = voxel_gi.ShowDebugBox();
        enabled = voxel_gi.Enabled();
        DEV_ASSERT_MSG(counter++ == 0, "Only support one ");
    }

    if (show_debug) {
        AddDebugCube(p_framedata, p_framedata.voxel_gi_bound, Color(0.5f, 0.3f, 0.6f, 0.5f));
    }

    auto& cache = p_framedata.perFrameCache;
    cache.c_enableVxgi = enabled;
    // @HACK: DONT use pass_idx
    if (!enabled) {
        p_framedata.voxelPass.pass_idx = -1;
        return;
    }
    p_framedata.voxelPass.pass_idx = 0;

    // @TODO: refactor the following
    const int voxel_texture_size = p_framedata.options.voxelTextureSize;
    DEV_ASSERT(IsPowerOfTwo(voxel_texture_size));
    DEV_ASSERT(voxel_texture_size <= 256);

    const auto voxel_world_center = p_framedata.voxel_gi_bound.Center();
    auto voxel_world_size = p_framedata.voxel_gi_bound.Size().x;

    const float texel_size = 1.0f / static_cast<float>(voxel_texture_size);
    const float voxel_size = voxel_world_size * texel_size;

    // this code is a bit mess here
    cache.c_voxelWorldCenter = voxel_world_center;
    cache.c_voxelWorldSizeHalf = 0.5f * voxel_world_size;
    cache.c_texelSize = texel_size;
    cache.c_voxelSize = voxel_size;
}

static void FillMainPass(const Scene& p_scene, FrameData& p_framedata) {
    const auto& camera = p_framedata.mainCamera;
    Frustum camera_frustum(camera.projectionMatrixFrustum * camera.viewMatrix);

    // main pass
    PerPassConstantBuffer pass_constant;
    pass_constant.c_viewMatrix = camera.viewMatrix;
    pass_constant.c_projectionMatrix = camera.projectionMatrixRendering;

    p_framedata.mainPass.pass_idx = static_cast<int>(p_framedata.passCache.size());
    p_framedata.passCache.emplace_back(pass_constant);

    using FilterFunc = std::function<bool(const AABB&)>;
    FilterFunc filter_main = [&](const AABB& p_aabb) -> bool { return camera_frustum.Intersects(p_aabb); };

    const bool is_opengl = p_framedata.options.isOpengl;
    for (auto [entity, obj] : p_scene.m_ObjectComponents) {
        const bool is_renderable = obj.flags & ObjectComponent::FLAG_RENDERABLE;
        const bool is_transparent = obj.flags & ObjectComponent::FLAG_TRANSPARENT;
        const bool is_opaque = is_renderable && !is_transparent;

        // @TODO: cast shadow

        const TransformComponent& transform = *p_scene.GetComponent<TransformComponent>(entity);
        DEV_ASSERT(p_scene.Contains<TransformComponent>(entity));
        const MeshComponent& mesh = *p_scene.GetComponent<MeshComponent>(obj.meshId);
        DEV_ASSERT(p_scene.Contains<MeshComponent>(obj.meshId));

        const Matrix4x4f& world_matrix = transform.GetWorldMatrix();
        AABB aabb = mesh.localBound;
        aabb.ApplyMatrix(world_matrix);

        PerBatchConstantBuffer batch_buffer;
        batch_buffer.c_worldMatrix = world_matrix;
        batch_buffer.c_meshFlag = mesh.armatureId.IsValid();

        DrawCommand draw;
        // @TODO: refactor the stencil part
        if (entity == p_scene.m_selected) {
            draw.flags = STENCIL_FLAG_SELECTED;
        }

        if (mesh.armatureId.IsValid()) {
            auto& armature = *p_scene.GetComponent<ArmatureComponent>(mesh.armatureId);
            DEV_ASSERT(armature.boneTransforms.size() <= MAX_BONE_COUNT);

            BoneConstantBuffer bone;
            memcpy(bone.c_bones, armature.boneTransforms.data(), sizeof(Matrix4x4f) * armature.boneTransforms.size());

            // @TODO: better memory usage
            draw.bone_idx = p_framedata.boneCache.FindOrAdd(mesh.armatureId, bone);
        } else {
            draw.bone_idx = -1;
        }

        draw.mat_idx = -1;
        draw.batch_idx = p_framedata.batchCache.FindOrAdd(entity, batch_buffer);
        draw.indexCount = static_cast<uint32_t>(mesh.indices.size());
        draw.mesh_data = (GpuMesh*)mesh.gpuResource.get();
        DEV_ASSERT(draw.mesh_data);

        auto add_to_pass = [&](std::vector<RenderCommand>& p_commands, FilterFunc& p_filter, bool p_model_only) {
            if (!p_filter(aabb)) {
                return;
            }

            DrawCommand drawCmd = draw;
            if (p_model_only) {
                p_commands.emplace_back(RenderCommand::From(drawCmd));
                return;
            }

            for (const auto& subset : mesh.subsets) {
                AABB aabb2 = subset.local_bound;
                aabb2.ApplyMatrix(world_matrix);
                if (!p_filter(aabb2)) {
                    continue;
                }

                const MaterialComponent* material = p_scene.GetComponent<MaterialComponent>(subset.material_id);
                MaterialConstantBuffer material_buffer;
                FillMaterialConstantBuffer(is_opengl, material, material_buffer);

                drawCmd.indexCount = subset.index_count;
                drawCmd.indexOffset = subset.index_offset;
                drawCmd.mat_idx = p_framedata.materialCache.FindOrAdd(subset.material_id, material_buffer);

                p_commands.emplace_back(RenderCommand::From(drawCmd));
            }
        };

        if (is_opaque) {
            add_to_pass(p_framedata.prepass_commands, filter_main, true);
        }

        if (is_opaque) {
            add_to_pass(p_framedata.gbuffer_commands, filter_main, false);
        }

        if (is_transparent) {
            add_to_pass(p_framedata.transparent_commands, filter_main, false);
        }

        if (p_framedata.voxel_gi_bound.IsValid()) {
            FilterFunc gi_filter = [&](const AABB& p_aabb) -> bool { return p_framedata.voxel_gi_bound.Intersects(p_aabb); };
            add_to_pass(p_framedata.voxelization_commands, gi_filter, false);
        }
    }
}

void RunMeshRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    FillLightBuffer(p_scene, p_framedata);
    FillVoxelPass(p_scene, p_framedata);
    FillMainPass(p_scene, p_framedata);
}

// @TODO: fix emitter
#if 0
static void FillMeshEmitterBuffer(const Scene& p_scene,
                                  RenderSystem& p_out_data) {
    for (auto [id, emitter] : p_scene.m_MeshEmitterComponents) {
        auto transform = p_scene.GetComponent<TransformComponent>(id);
        auto mesh = p_scene.GetComponent<MeshComponent>(emitter.meshId);
        if (DEV_VERIFY(transform && mesh)) {
            PerBatchConstantBuffer batch_buffer;
            batch_buffer.c_worldMatrix = Matrix4x4f(1);
            batch_buffer.c_meshFlag = MESH_HAS_INSTANCE;

            BatchContext draw;

            auto& position_buffer = p_out_data.boneCache.buffer;

            InstanceContext instance;
            instance.gpuMesh = mesh->gpuResource.get();
            instance.indexCount = (uint32_t)mesh->indices.size();
            instance.indexOffset = 0;
            instance.instanceCount = (uint32_t)emitter.aliveList.size();
            instance.batchIdx = p_out_data.batchCache.FindOrAdd(id, batch_buffer);
            instance.instanceBufferIndex = (int)position_buffer.size();
            auto material_id = mesh->subsets[0].material_id;
            auto material = p_scene.GetComponent<MaterialComponent>(material_id);
            DEV_ASSERT(material);
            MaterialConstantBuffer material_buffer;
            FillMaterialConstantBuffer(p_out_data.options.isOpengl, material, material_buffer);
            instance.materialIdx = p_out_data.materialCache.FindOrAdd(material_id, material_buffer);

            // @HACK: use bone cache
            DEV_ASSERT(instance.instanceCount <= MAX_BONE_COUNT);
            position_buffer.resize(position_buffer.size() + 1);
            auto& gpu_buffer = position_buffer.back();
            int i = 0;
            for (auto index : emitter.aliveList) {
                const auto& p = emitter.particles[index.v];

                Matrix4x4f translation = Translate(p.position);
                Matrix4x4f scale = Scale(Vector3f(p.scale));
                Matrix4x4f rotation = glm::toMat4(glm::quat(glm::vec3(p.rotation.x, p.rotation.y, p.rotation.z)));
                gpu_buffer.c_bones[i++] = translation * rotation * scale;
            }

            p_out_data.instances.push_back(instance);
        }
    }
}

static void FillParticleEmitterBuffer(const Scene& p_scene,
                                      RenderSystem& p_out_data) {
    // @TODO: engine->get frame
    static int s_counter = -1;
    s_counter++;

    const auto view = p_scene.View<ParticleEmitterComponent>();
    for (auto [id, emitter] : view) {
        const uint32_t pre_sim_idx = emitter.GetPreIndex();
        const uint32_t post_sim_idx = emitter.GetPostIndex();
        EmitterConstantBuffer buffer;
        const TransformComponent* transform = p_scene.GetComponent<TransformComponent>(id);
        buffer.c_preSimIdx = pre_sim_idx;
        buffer.c_postSimIdx = post_sim_idx;
        buffer.c_elapsedTime = p_scene.m_timestep;
        buffer.c_lifeSpan = emitter.particleLifeSpan;
        buffer.c_seeds = Vector3f(Random::Float(), Random::Float(), Random::Float());
        buffer.c_emitterScale = emitter.particleScale;
        buffer.c_emitterPosition = transform->GetTranslation();
        buffer.c_particlesPerFrame = emitter.particlesPerFrame;
        buffer.c_emitterStartingVelocity = emitter.startingVelocity;
        buffer.c_emitterMaxParticleCount = emitter.maxParticleCount;
        buffer.c_emitterHasGravity = emitter.gravity;

        buffer.c_particleColor = emitter.color;
        buffer.c_emitterUseTexture = !emitter.texture.empty();
        buffer.c_subUvCounter = s_counter;

        p_out_data.emitterCache.push_back(buffer);
        p_out_data.emitters.emplace_back(emitter);
    }
}
#endif

}  // namespace my
