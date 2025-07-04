#include "render_system.h"

#include "engine/core/base/random.h"
#include "engine/math/frustum.h"
#include "engine/math/matrix_transform.h"
#include "engine/renderer/graphics_defines.h"
#include "engine/renderer/render_graph/render_graph.h"
#include "engine/scene/scene.h"

// @TODO: remove
// #include "engine/math/matrix_transform.h"
#include "engine/renderer/base_graphics_manager.h"
#include "engine/renderer/path_tracer/bvh_accel.h"
#include "engine/runtime/asset_registry.h"
#include "engine/runtime/input_manager.h"

namespace my::renderer {

using my::AABB;
using my::Frustum;

RenderSystem::RenderSystem(const RenderOptions& p_options) : options(p_options) {
    m_renderGraph = IGraphicsManager::GetSingleton().GetActiveRenderGraph();
    DEV_ASSERT(m_renderGraph);
}

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

void RenderSystem::FillPass(const Scene& p_scene,
                            PassContext&,
                            FilterObjectFunc1 p_filter1,
                            FilterObjectFunc2 p_filter2,
                            DrawPass* p_draw_pass,
                            bool p_use_material) {

    const bool is_opengl = this->options.isOpengl;
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

        draw.batch_idx = this->batchCache.FindOrAdd(entity, batch_buffer);
        if (mesh.armatureId.IsValid()) {
            auto& armature = *p_scene.GetComponent<ArmatureComponent>(mesh.armatureId);
            DEV_ASSERT(armature.boneTransforms.size() <= MAX_BONE_COUNT);

            BoneConstantBuffer bone;
            memcpy(bone.c_bones, armature.boneTransforms.data(), sizeof(Matrix4x4f) * armature.boneTransforms.size());

            // @TODO: better memory usage
            draw.bone_idx = this->boneCache.FindOrAdd(mesh.armatureId, bone);
        } else {
            draw.bone_idx = -1;
        }

        // HACK
        draw.mesh_data = (GpuMesh*)mesh.gpuResource.get();
        draw.mat_idx = -1;
        DEV_ASSERT(draw.mesh_data);

        if (!p_use_material) {
            draw.indexCount = static_cast<uint32_t>(mesh.indices.size());
            p_draw_pass->AddCommand(RenderCommand::from(draw));
            // @TODO: add to pass
        } else {
            for (const auto& subset : mesh.subsets) {
                aabb = subset.local_bound;
                aabb.ApplyMatrix(world_matrix);
                if (!p_filter2(aabb)) {
                    continue;
                }

                const MaterialComponent* material = p_scene.GetComponent<MaterialComponent>(subset.material_id);
                MaterialConstantBuffer material_buffer;
                FillMaterialConstantBuffer(is_opengl, material, material_buffer);

                // Draw submesh if pass cares about material
                DrawCommand draw2 = draw;
                draw2.indexCount = subset.index_count;
                draw2.indexOffset = subset.index_offset;
                draw2.mat_idx = this->materialCache.FindOrAdd(subset.material_id, material_buffer);

                p_draw_pass->AddCommand(RenderCommand::from(draw2));
            }
        }
    }
}

static void DebugDrawBVH(int p_level, BvhAccel* p_bvh, const Matrix4x4f* p_matrix) {
    if (!p_bvh || p_bvh->depth > p_level) {
        return;
    }

    if (p_bvh->depth == p_level) {
        renderer::AddDebugCube(p_bvh->aabb,
                               Color::HexRgba(0xFFFF0037),
                               p_matrix);
    }

    DebugDrawBVH(p_level, p_bvh->left.get(), p_matrix);
    DebugDrawBVH(p_level, p_bvh->right.get(), p_matrix);
};

static void FillConstantBuffer(const Scene& p_scene, RenderSystem& p_out_data) {
    const auto& options = p_out_data.options;
    auto& cache = p_out_data.perFrameCache;

    // camera
    {
        const auto& camera = p_out_data.mainCamera;
        cache.c_invView = glm::inverse(camera.viewMatrix);
        cache.c_invProjection = glm::inverse(camera.projectionMatrixRendering);
        cache.c_cameraFovDegree = camera.fovy.GetDegree();
        cache.c_cameraForward = camera.front;
        cache.c_cameraRight = camera.right;
        cache.c_cameraUp = camera.up;
        cache.c_cameraPosition = camera.position;
    }

    // Bloom
    {
        cache.c_bloomThreshold = 1.3f;
        cache.c_enableBloom = options.bloomEnabled;

        cache.c_debugVoxelId = options.debugVoxelId;
        cache.c_ptObjectCount = (int)p_scene.m_ObjectComponents.GetCount();
    }

    // SSAO
    {
        cache.c_ssaoEnabled = options.ssaoEnabled;
        cache.c_ssaoKernalRadius = options.ssaoKernelRadius;
        const auto& kernel_data = renderer::GetKernelData();
        constexpr size_t kernel_size = sizeof(renderer::KernelData);
        static_assert(sizeof(cache.c_ssaoKernel) == kernel_size);
        memcpy(cache.c_ssaoKernel, kernel_data.data(), kernel_size);
    }

    // Sky
    {
    }

    // @TODO: refactor
    static int s_frameIndex = 0;
    cache.c_frameIndex = s_frameIndex++;
    // @TODO: fix this
    cache.c_sceneDirty = p_scene.GetDirtyFlags() != SCENE_DIRTY_NONE;

    // Force fields
    int counter = 0;
    for (auto [id, force_field_component] : p_scene.m_ForceFieldComponents) {
        ForceField& force_field = cache.c_forceFields[counter++];
        const TransformComponent& transform = *p_scene.GetComponent<TransformComponent>(id);
        force_field.position = transform.GetTranslation();
        force_field.strength = force_field_component.strength;
    }

    cache.c_forceFieldsCount = counter;

    // @TODO: cache the slots
    // Texture indices
    auto find_index = [](RenderTargetResourceName p_name) -> uint32_t {
        std::shared_ptr<GpuTexture> resource = IGraphicsManager::GetSingleton().FindTexture(p_name);
        if (!resource) {
            return 0;
        }

        return static_cast<uint32_t>(resource->GetResidentHandle());
    };

    // @TODO: opengl doesn't really uses it, consider use 32 bit for handle
    cache.c_GbufferBaseColorMapResidentHandle.Set32(find_index(RESOURCE_GBUFFER_BASE_COLOR));
    cache.c_GbufferNormalMapResidentHandle.Set32(find_index(RESOURCE_GBUFFER_NORMAL));
    cache.c_GbufferMaterialMapResidentHandle.Set32(find_index(RESOURCE_GBUFFER_MATERIAL));
    cache.c_GbufferDepthResidentHandle.Set32(find_index(RESOURCE_GBUFFER_DEPTH));
    cache.c_PointShadowArrayResidentHandle.Set32(find_index(RESOURCE_POINT_SHADOW_CUBE_ARRAY));
    cache.c_SsaoMapResidentHandle.Set32(find_index(RESOURCE_SSAO));

    cache.c_ShadowMapResidentHandle.Set32(find_index(RESOURCE_SHADOW_MAP));

    cache.c_TextureHighlightSelectResidentHandle.Set32(find_index(RESOURCE_OUTLINE_SELECT));
    cache.c_TextureLightingResidentHandle.Set32(find_index(RESOURCE_LIGHTING));

    // @TODO: fix
    for (auto const [entity, environment] : p_scene.View<EnvironmentComponent>()) {
        cache.c_ambientColor = environment.ambient.color;
        if (!environment.sky.texturePath.empty()) {
            environment.sky.textureAsset = AssetRegistry::GetSingleton().GetAssetByHandle<ImageAsset>(environment.sky.texturePath);
        }

        if (auto asset = environment.sky.textureAsset; asset && asset->gpu_texture) {
            p_out_data.skyboxHdr = asset->gpu_texture;

            // @TODO: fix this
            g_constantCache.cache.c_hdrEnvMap.Set64(asset->gpu_texture->GetResidentHandle());
            g_constantCache.update();
        }
    }

    // @TODO:
    const int level = options.debugBvhDepth;
    if (level > -1) {
        for (auto const [id, obj] : p_scene.m_ObjectComponents) {
            const MeshComponent* mesh = p_scene.GetComponent<MeshComponent>(obj.meshId);
            const TransformComponent* transform = p_scene.GetComponent<TransformComponent>(id);
            if (mesh && transform) {
                if (const auto& bvh = mesh->bvh; bvh) {
                    const auto& matrix = transform->GetWorldMatrix();
                    DebugDrawBVH(level, bvh.get(), &matrix);
                }
            }
        }
    }
}

void RenderSystem::FillLightBuffer(const Scene& p_scene) {
    const uint32_t light_count = glm::min<uint32_t>((uint32_t)p_scene.GetCount<LightComponent>(), MAX_LIGHT_COUNT);

    auto& cache = this->perFrameCache;
    cache.c_lightCount = light_count;

    [[maybe_unused]] auto& point_shadow_cache = this->pointShadowCache;

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

                if (this->options.isOpengl) {
                    light.projection_matrix = BuildOpenGlOrthoRH(-size, size, -size, size, -size, 3.0f * size);
                } else {
                    light.projection_matrix = BuildOrthoRH(-size, size, -size, size, -size, 3.0f * size);
                }

                PerPassConstantBuffer pass_constant;
                // @TODO: Build correct matrices
                pass_constant.c_projectionMatrix = light.projection_matrix;
                pass_constant.c_viewMatrix = light.view_matrix;
                this->shadowPasses[0].pass_idx = static_cast<int>(this->passCache.size());
                this->passCache.emplace_back(pass_constant);

                auto pass = m_renderGraph->FindPass(RenderPassName::SHADOW);
                if (!pass) {
                    continue;
                }
                auto res = pass->FindDrawPass("ShadowDrawPass");
                DEV_ASSERT(res.has_value());

                Frustum light_frustum(light.projection_matrix * light.view_matrix);
                FillPass(
                    p_scene,
                    this->shadowPasses[0],
                    [](const ObjectComponent& p_object) {
                        return p_object.flags & ObjectComponent::FLAG_CAST_SHADOW;
                    },
                    [&](const AABB& p_aabb) {
                        return light_frustum.Intersects(p_aabb);
                    },
                    *res, false);
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

void RenderSystem::FillVoxelPass(const Scene& p_scene) {
    bool enabled = false;
    bool show_debug = false;
    voxel_gi_bound.MakeInvalid();
    int counter = 0;
    for (auto [entity, voxel_gi] : p_scene.m_VoxelGiComponents) {
        voxel_gi_bound = voxel_gi.region;
        if (!voxel_gi_bound.IsValid()) {
            return;
        }

        show_debug = voxel_gi.ShowDebugBox();
        enabled = voxel_gi.Enabled();
        DEV_ASSERT_MSG(counter++ == 0, "Only support one ");
    }

    if (show_debug) {
        renderer::AddDebugCube(voxel_gi_bound, Color(0.5f, 0.3f, 0.6f, 0.5f));
    }

    auto& cache = this->perFrameCache;
    cache.c_enableVxgi = enabled;
    // @HACK: DONT use pass_idx
    if (!enabled) {
        this->voxelPass.pass_idx = -1;
        return;
    }
    this->voxelPass.pass_idx = 0;

    // @TODO: refactor the following
    const int voxel_texture_size = this->options.voxelTextureSize;
    DEV_ASSERT(IsPowerOfTwo(voxel_texture_size));
    DEV_ASSERT(voxel_texture_size <= 256);

    const auto voxel_world_center = voxel_gi_bound.Center();
    auto voxel_world_size = voxel_gi_bound.Size().x;

    const float texel_size = 1.0f / static_cast<float>(voxel_texture_size);
    const float voxel_size = voxel_world_size * texel_size;

    // this code is a bit mess here
    cache.c_voxelWorldCenter = voxel_world_center;
    cache.c_voxelWorldSizeHalf = 0.5f * voxel_world_size;
    cache.c_texelSize = texel_size;
    cache.c_voxelSize = voxel_size;
}

void RenderSystem::FillMainPass(const Scene& p_scene) {
    const auto& camera = this->mainCamera;
    Frustum camera_frustum(camera.projectionMatrixFrustum * camera.viewMatrix);

    // main pass
    PerPassConstantBuffer pass_constant;
    pass_constant.c_viewMatrix = camera.viewMatrix;
    pass_constant.c_projectionMatrix = camera.projectionMatrixRendering;

    this->mainPass.pass_idx = static_cast<int>(this->passCache.size());
    this->passCache.emplace_back(pass_constant);

    auto tmp = m_renderGraph->FindPass(RenderPassName::GBUFFER);
    DrawPass* gbuffer_pass = tmp ? *tmp->FindDrawPass("GbufferDrawPass") : nullptr;
    tmp = m_renderGraph->FindPass(RenderPassName::VOXELIZATION);
    DrawPass* voxelization_pass = tmp ? *tmp->FindDrawPass("VoxelizationDrawPass") : nullptr;
    tmp = m_renderGraph->FindPass(RenderPassName::PREPASS);
    DrawPass* early_z_pass = tmp ? *tmp->FindDrawPass("EarlyZDrawPass") : nullptr;
    tmp = m_renderGraph->FindPass(RenderPassName::FORWARD);
    // @TODO: should separate it from forward?
    DrawPass* transparent_pass = tmp ? *tmp->FindDrawPass("ForwardDrawPass") : nullptr;

    using FilterFunc = std::function<bool(const AABB&)>;
    FilterFunc filter_main = [&](const AABB& p_aabb) -> bool { return camera_frustum.Intersects(p_aabb); };

    const bool is_opengl = this->options.isOpengl;
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
            draw.bone_idx = this->boneCache.FindOrAdd(mesh.armatureId, bone);
        } else {
            draw.bone_idx = -1;
        }

        draw.mat_idx = -1;
        draw.batch_idx = this->batchCache.FindOrAdd(entity, batch_buffer);
        draw.indexCount = static_cast<uint32_t>(mesh.indices.size());
        draw.mesh_data = (GpuMesh*)mesh.gpuResource.get();
        DEV_ASSERT(draw.mesh_data);

        auto add_to_pass = [&](DrawPass* p_pass, FilterFunc& p_filter, bool p_model_only) {
            if (!p_filter(aabb)) {
                return;
            }

            DrawCommand drawCmd = draw;
            if (p_model_only) {
                p_pass->AddCommand(RenderCommand::from(drawCmd));
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
                drawCmd.mat_idx = this->materialCache.FindOrAdd(subset.material_id, material_buffer);

                p_pass->AddCommand(RenderCommand::from(drawCmd));
            }
        };

        if (is_opaque && early_z_pass) {
            add_to_pass(early_z_pass, filter_main, true);
        }

        if (is_opaque && gbuffer_pass) {
            add_to_pass(gbuffer_pass, filter_main, false);
        }

        if (is_transparent && transparent_pass) {
            add_to_pass(transparent_pass, filter_main, false);
        }

        if (voxelization_pass && this->voxel_gi_bound.IsValid()) {
            FilterFunc gi_filter = [&](const AABB& p_aabb) -> bool { return this->voxel_gi_bound.Intersects(p_aabb); };
            add_to_pass(voxelization_pass, gi_filter, false);
        }
    }
}

static void FillEnvConstants(const Scene&,
                             RenderSystem& p_out_data) {
    // @TODO: return if necessary

    constexpr int count = IBL_MIP_CHAIN_MAX * 6;
    if (p_out_data.batchCache.buffer.size() < count) {
        p_out_data.batchCache.buffer.resize(count);
    }

    auto matrices = p_out_data.options.isOpengl ? BuildOpenGlCubeMapViewProjectionMatrix(Vector3f(0)) : BuildCubeMapViewProjectionMatrix(Vector3f(0));
    for (int mip_idx = 0; mip_idx < IBL_MIP_CHAIN_MAX; ++mip_idx) {
        for (int face_id = 0; face_id < 6; ++face_id) {
            auto& batch = p_out_data.batchCache.buffer[mip_idx * 6 + face_id];
            batch.c_cubeProjectionViewMatrix = matrices[face_id];
            batch.c_envPassRoughness = (float)mip_idx / (float)(IBL_MIP_CHAIN_MAX - 1);
        }
    }
}

static void FillBloomConstants(const Scene& p_config,
                               RenderSystem& p_out_data) {
    unused(p_config);

    auto& gm = IGraphicsManager::GetSingleton();
    auto image = gm.FindTexture(RESOURCE_BLOOM_0).get();
    if (!image) {
        return;
    }
    constexpr int count = BLOOM_MIP_CHAIN_MAX * 2 - 1;
    if (p_out_data.batchCache.buffer.size() < count) {
        p_out_data.batchCache.buffer.resize(count);
    }

    int offset = 0;
    p_out_data.batchCache.buffer[offset++].c_BloomOutputImageResidentHandle.Set32((uint)image->GetUavHandle());

    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX - 1; ++i) {
        auto input = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i + 1));

        p_out_data.batchCache.buffer[i + offset].c_BloomInputTextureResidentHandle.Set32((uint)input->GetResidentHandle());
        p_out_data.batchCache.buffer[i + offset].c_BloomOutputImageResidentHandle.Set32((uint)output->GetUavHandle());
    }

    offset += BLOOM_MIP_CHAIN_MAX - 1;

    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto input = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));

        p_out_data.batchCache.buffer[i - 1 + offset].c_BloomInputTextureResidentHandle.Set32((uint)input->GetResidentHandle());
        p_out_data.batchCache.buffer[i - 1 + offset].c_BloomOutputImageResidentHandle.Set32((uint)output->GetUavHandle());
    }
}

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

void PrepareRenderData(const PerspectiveCameraComponent& p_camera,
                       const Scene& p_scene,
                       RenderSystem& p_out_data) {
    // fill camera
    {
        auto reverse_z = [](Matrix4x4f& p_perspective) {
            constexpr Matrix4x4f matrix{ 1.0f, 0.0f, 0.0f, 0.0f,
                                         0.0f, 1.0f, 0.0f, 0.0f,
                                         0.0f, 0.0f, -1.0f, 0.0f,
                                         0.0f, 0.0f, 1.0f, 1.0f };
            p_perspective = matrix * p_perspective;
        };
        auto normalize_unit_range = [](Matrix4x4f& p_perspective) {
            constexpr Matrix4x4f matrix{ 1.0f, 0.0f, 0.0f, 0.0f,
                                         0.0f, 1.0f, 0.0f, 0.0f,
                                         0.0f, 0.0f, 0.5f, 0.0f,
                                         0.0f, 0.0f, 0.5f, 1.0f };
            p_perspective = matrix * p_perspective;
        };

        auto& camera = p_out_data.mainCamera;
        camera.sceenWidth = static_cast<float>(p_camera.GetWidth());
        camera.sceenHeight = static_cast<float>(p_camera.GetHeight());
        camera.aspectRatio = camera.sceenWidth / camera.sceenHeight;
        camera.fovy = p_camera.GetFovy();
        camera.zNear = p_camera.GetNear();
        camera.zFar = p_camera.GetFar();

        camera.viewMatrix = p_camera.GetViewMatrix();
        camera.projectionMatrixFrustum = p_camera.GetProjectionMatrix();
        // @TODO: check out this
        // https://tomhultonharrop.com/mathematics/graphics/2023/08/06/reverse-z.html
        if (p_out_data.options.isOpengl) {
            camera.projectionMatrixRendering = BuildOpenGlPerspectiveRH(
                camera.fovy.GetRadians(),
                camera.aspectRatio,
                camera.zNear,
                camera.zFar);
            normalize_unit_range(camera.projectionMatrixRendering);
            reverse_z(camera.projectionMatrixRendering);
        } else {
            camera.projectionMatrixRendering = BuildPerspectiveRH(
                camera.fovy.GetRadians(),
                camera.aspectRatio,
                camera.zNear,
                camera.zFar);
            reverse_z(camera.projectionMatrixRendering);
        }
        camera.position = p_camera.GetPosition();

        camera.front = p_camera.GetFront();
        camera.right = p_camera.GetRight();
        camera.up = cross(camera.front, camera.right);
    }

    FillConstantBuffer(p_scene, p_out_data);
    p_out_data.FillLightBuffer(p_scene);
    p_out_data.FillVoxelPass(p_scene);
    p_out_data.FillMainPass(p_scene);

    FillMeshEmitterBuffer(p_scene, p_out_data);
    FillBloomConstants(p_scene, p_out_data);
    FillEnvConstants(p_scene, p_out_data);
    FillParticleEmitterBuffer(p_scene, p_out_data);
}

}  // namespace my::renderer
