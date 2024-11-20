#include "graphics_manager.h"

#include "core/base/random.h"
#include "core/debugger/profiler.h"
#include "core/math/frustum.h"
#include "core/math/matrix_transform.h"
#if USING(PLATFORM_WINDOWS)
#include "drivers/d3d11/d3d11_graphics_manager.h"
#include "drivers/d3d12/d3d12_graphics_manager.h"
#endif
#include "drivers/empty/empty_graphics_manager.h"
#include "drivers/opengl/opengl_graphics_manager.h"
#include "rendering/graphics_dvars.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/render_manager.h"
#include "shader_resource_defines.hlsl.h"

// @TODO: refactor
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace my {

template<typename T>
static auto CreateUniformCheckSize(GraphicsManager& p_graphics_manager, uint32_t p_max_count) {
    static_assert(sizeof(T) % 256 == 0);
    GpuBufferDesc buffer_desc{};
    buffer_desc.slot = T::GetUniformBufferSlot();
    buffer_desc.elementCount = p_max_count;
    buffer_desc.elementSize = sizeof(T);
    return p_graphics_manager.CreateConstantBuffer(buffer_desc);
}

ConstantBuffer<PerSceneConstantBuffer> g_constantCache;
ConstantBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;
ConstantBuffer<EnvConstantBuffer> g_env_cache;

// @TODO: refactor this
template<typename T>
static void CreateUniformBuffer(ConstantBuffer<T>& p_buffer) {
    GpuBufferDesc buffer_desc{};
    buffer_desc.slot = T::GetUniformBufferSlot();
    buffer_desc.elementCount = 1;
    buffer_desc.elementSize = sizeof(T);
    p_buffer.buffer = GraphicsManager::GetSingleton().CreateConstantBuffer(buffer_desc);
}

bool GraphicsManager::Initialize() {
    m_enableValidationLayer = DVAR_GET_BOOL(gfx_gpu_validation);

    const int num_frames = (GetBackend() == Backend::D3D12) ? NUM_FRAMES_IN_FLIGHT : 1;
    m_frameContexts.resize(num_frames);
    for (int i = 0; i < num_frames; ++i) {
        m_frameContexts[i] = CreateFrameContext();
    }

    if (!InitializeImpl()) {
        return false;
    }

    for (int i = 0; i < num_frames; ++i) {
        FrameContext& frame_context = *m_frameContexts[i].get();
        frame_context.batchCb = ::my::CreateUniformCheckSize<PerBatchConstantBuffer>(*this, 4096 * 16);
        frame_context.passCb = ::my::CreateUniformCheckSize<PerPassConstantBuffer>(*this, 32);
        frame_context.materialCb = ::my::CreateUniformCheckSize<MaterialConstantBuffer>(*this, 2048 * 16);
        frame_context.boneCb = ::my::CreateUniformCheckSize<BoneConstantBuffer>(*this, 16);
        frame_context.emitterCb = ::my::CreateUniformCheckSize<EmitterConstantBuffer>(*this, 32);
        frame_context.pointShadowCb = ::my::CreateUniformCheckSize<PointShadowConstantBuffer>(*this, 6 * MAX_POINT_LIGHT_SHADOW_COUNT);
        frame_context.perFrameCb = ::my::CreateUniformCheckSize<PerFrameConstantBuffer>(*this, 1);
    }

    // @TODO: refactor
    CreateUniformBuffer<PerSceneConstantBuffer>(g_constantCache);
    CreateUniformBuffer<DebugDrawConstantBuffer>(g_debug_draw_cache);
    CreateUniformBuffer<EnvConstantBuffer>(g_env_cache);

    DEV_ASSERT(m_pipelineStateManager);

    if (!m_pipelineStateManager->Initialize()) {
        return false;
    }

    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot) {
        std::shared_ptr<GpuTexture> texture = FindTexture(p_name);
        if (!texture) {
            return;
        }

        DEV_ASSERT(p_slot >= 0);
        texture->slot = p_slot;
    };
#define SRV(TYPE, NAME, SLOT, BINDING) bind_slot(BINDING, SLOT);
    SRV_LIST
#undef SRV

    return true;
}

void GraphicsManager::EventReceived(std::shared_ptr<Event> event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(event.get()); e) {
        OnSceneChange(*e->GetScene());
    }
    if (ResizeEvent* e = dynamic_cast<ResizeEvent*>(event.get()); e) {
        OnWindowResize(e->GetWidth(), e->GetHeight());
    }
}

std::shared_ptr<GraphicsManager> GraphicsManager::Create() {
    const std::string& backend = DVAR_GET_STRING(gfx_backend);

#if USING(PLATFORM_APPLE)
    ERR_FAIL_COND_V_MSG(backend != "metal", nullptr, "only support Metal backend on Apple");
    return std::make_shared<EmptyGraphicsManager>("MetalGraphicsManager", Backend::METAL, 1);
#else
#if USING(PLATFORM_WINDOWS)
    if (backend == "d3d11") {
        return std::make_shared<D3d11GraphicsManager>();
    }
    if (backend == "d3d12") {
        return std::make_shared<D3d12GraphicsManager>();
    }
#else
#error Platform not supported
#endif
    if (backend == "opengl") {
        return std::make_shared<OpenGlGraphicsManager>();
    }

    return std::make_shared<EmptyGraphicsManager>("EmptyGraphicsmanager", Backend::EMPTY, 1);
#endif
}

void GraphicsManager::SetPipelineState(PipelineStateName p_name) {
    SetPipelineStateImpl(p_name);
}

void GraphicsManager::RequestTexture(ImageHandle* p_handle, OnTextureLoadFunc p_func) {
    m_loadedImages.push(ImageTask{ p_handle, p_func });
}

void GraphicsManager::Update(Scene& p_scene) {
    OPTICK_EVENT();

    Cleanup();

    UpdateConstants(p_scene);
    UpdateEmitters(p_scene);
    UpdateForceFields(p_scene);
    UpdateLights(p_scene);
    UpdateVoxelPass(p_scene);
    UpdateMainPass(p_scene);

    // @TODO: make it a function
    auto loaded_images = m_loadedImages.pop_all();
    while (!loaded_images.empty()) {
        auto task = loaded_images.front();
        loaded_images.pop();
        DEV_ASSERT(task.handle->state == ASSET_STATE_READY);
        Image* image = task.handle->Get();
        DEV_ASSERT(image);

        GpuTextureDesc texture_desc{};
        SamplerDesc sampler_desc{};
        renderer::fill_texture_and_sampler_desc(image, texture_desc, sampler_desc);

        image->gpu_texture = CreateTexture(texture_desc, sampler_desc);
        if (task.func) {
            task.func(task.handle->Get());
        }
    }

    {
        OPTICK_EVENT("Render");
        BeginFrame();

        auto& frame = GetCurrentFrame();
        UpdateConstantBuffer(frame.batchCb.get(), frame.batchCache.buffer);
        UpdateConstantBuffer(frame.materialCb.get(), frame.materialCache.buffer);
        UpdateConstantBuffer(frame.boneCb.get(), frame.boneCache.buffer);
        UpdateConstantBuffer(frame.passCb.get(), frame.passCache);
        UpdateConstantBuffer(frame.emitterCb.get(), frame.emitterCache);
        UpdateConstantBuffer<PointShadowConstantBuffer, 6 * MAX_POINT_LIGHT_SHADOW_COUNT>(frame.pointShadowCb.get(), frame.pointShadowCache);
        UpdateConstantBuffer(frame.perFrameCb.get(), &frame.perFrameCache, sizeof(PerFrameConstantBuffer));
        BindConstantBufferSlot<PerFrameConstantBuffer>(frame.perFrameCb.get(), 0);

        m_renderGraph.Execute(*this);
        Render();
        EndFrame();
        Present();
        MoveToNextFrame();
    }
}

void GraphicsManager::BeginFrame() {
}

void GraphicsManager::EndFrame() {
}

void GraphicsManager::MoveToNextFrame() {
}

std::unique_ptr<FrameContext> GraphicsManager::CreateFrameContext() {
    return std::make_unique<FrameContext>();
}

void GraphicsManager::BeginDrawPass(const DrawPass* p_draw_pass) {
    for (auto& texture : p_draw_pass->outputs) {
        if (texture->slot >= 0) {
            UnbindTexture(Dimension::TEXTURE_2D, texture->slot);
            // RT_DEBUG("  -- unbound resource '{}'({})", RenderTargetResourceNameToString(it->desc.name), it->slot);
        }
    }
}

void GraphicsManager::EndDrawPass(const DrawPass* p_draw_pass) {
    UnsetRenderTarget();
    for (auto& texture : p_draw_pass->outputs) {
        if (texture->slot >= 0) {
            BindTexture(Dimension::TEXTURE_2D, texture->GetHandle(), texture->slot);
            // RT_DEBUG("  -- bound resource '{}'({})", RenderTargetResourceNameToString(it->desc.name), it->slot);
        }
    }
}

void GraphicsManager::SelectRenderGraph() {
    std::string method(DVAR_GET_STRING(gfx_render_graph));
    const std::map<std::string, RenderGraphName> lookup = {
        { "dummy", RenderGraphName::DUMMY },
        { "vxgi", RenderGraphName::VXGI },
    };

    auto it = lookup.find(method);
    if (it == lookup.end()) {
        m_renderGraphName = RenderGraphName::DEFAULT;
    } else {
        m_renderGraphName = it->second;
    }

    // force to default
    if (m_backend == Backend::D3D11 && m_renderGraphName == RenderGraphName::VXGI) {
        m_renderGraphName = RenderGraphName::DEFAULT;
    }
    if (m_backend == Backend::D3D12) {
        m_renderGraphName = RenderGraphName::DUMMY;
    }

    switch (m_renderGraphName) {
        case RenderGraphName::DUMMY:
            rg::RenderPassCreator::CreateDummy(m_renderGraph);
            break;
        case RenderGraphName::VXGI:
            rg::RenderPassCreator::CreateVxgi(m_renderGraph);
            break;
        default:
            rg::RenderPassCreator::CreateDefault(m_renderGraph);
            break;
    }
}

std::shared_ptr<GpuTexture> GraphicsManager::CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
    auto texture = CreateTextureImpl(p_texture_desc, p_sampler_desc);
    if (p_texture_desc.type != AttachmentType::NONE) {
        DEV_ASSERT(m_resourceLookup.find(p_texture_desc.name) == m_resourceLookup.end());
        m_resourceLookup[p_texture_desc.name] = texture;
    }
    return texture;
}

std::shared_ptr<GpuTexture> GraphicsManager::FindTexture(RenderTargetResourceName p_name) const {
    if (m_resourceLookup.empty()) {
        return nullptr;
    }

    auto it = m_resourceLookup.find(p_name);
    if (it == m_resourceLookup.end()) {
        return nullptr;
    }
    return it->second;
}

uint64_t GraphicsManager::GetFinalImage() const {
    const GpuTexture* texture = nullptr;
    switch (m_renderGraphName) {
        case RenderGraphName::DUMMY:
            texture = FindTexture(RESOURCE_TONE).get();
            break;
        case RenderGraphName::VXGI:
            texture = FindTexture(RESOURCE_FINAL).get();
            break;
        case RenderGraphName::DEFAULT:
            texture = FindTexture(RESOURCE_TONE).get();
            break;
        default:
            CRASH_NOW();
            break;
    }

    if (texture) {
        return texture->GetHandle();
    }

    return 0;
}

// @TODO: remove this
static void FillMaterialConstantBuffer(const MaterialComponent* material, MaterialConstantBuffer& cb) {
    cb.c_baseColor = material->baseColor;
    cb.c_metallic = material->metallic;
    cb.c_roughness = material->roughness;
    cb.c_emissivePower = material->emissive;

    auto set_texture = [&](int p_idx,
                           TextureHandle& p_out_handle,
                           uint& p_out_resident_handle) {
        p_out_handle = 0;
        p_out_resident_handle = 0;

        if (!material->textures[p_idx].enabled) {
            return false;
        }

        ImageHandle* handle = material->textures[p_idx].image;
        if (!handle) {
            return false;
        }

        Image* image = handle->Get();
        if (!image) {
            return false;
        }

        auto texture = reinterpret_cast<GpuTexture*>(image->gpu_texture.get());
        if (!texture) {
            return false;
        }

        p_out_handle = texture->GetHandle();
        p_out_resident_handle = static_cast<uint32_t>(texture->GetResidentHandle());
        return true;
    };

    cb.c_hasBaseColorMap = set_texture(MaterialComponent::TEXTURE_BASE, cb.c_baseColorMapHandle, cb.c_BaseColorMapIndex);
    cb.c_hasNormalMap = set_texture(MaterialComponent::TEXTURE_NORMAL, cb.c_normalMapHandle, cb.c_NormalMapIndex);
    cb.c_hasMaterialMap = set_texture(MaterialComponent::TEXTURE_METALLIC_ROUGHNESS, cb.c_materialMapHandle, cb.c_MaterialMapIndex);
}

void GraphicsManager::Cleanup() {
    auto& frame_context = GetCurrentFrame();
    frame_context.batchCache.Clear();
    frame_context.materialCache.Clear();
    frame_context.boneCache.Clear();
    frame_context.passCache.clear();
    frame_context.emitterCache.clear();

    for (auto& pass : m_shadowPasses) {
        pass.draws.clear();
    }

    for (auto& pass : m_pointShadowPasses) {
        pass.reset();
    }

    m_mainPass.draws.clear();
    m_voxelPass.draws.clear();
}

void GraphicsManager::UpdateConstants(const Scene& p_scene) {
    Camera& camera = *p_scene.m_camera.get();

    auto& cache = GetCurrentFrame().perFrameCache;
    cache.c_cameraPosition = camera.GetPosition();

    cache.c_enableVxgi = DVAR_GET_BOOL(gfx_enable_vxgi);
    cache.c_debugVoxelId = DVAR_GET_INT(gfx_debug_vxgi_voxel);
    cache.c_noTexture = DVAR_GET_BOOL(gfx_no_texture);

    // Bloom
    cache.c_bloomThreshold = DVAR_GET_FLOAT(gfx_bloom_threshold);
    cache.c_enableBloom = DVAR_GET_BOOL(gfx_enable_bloom);

    // @TODO: refactor the following
    const int voxel_texture_size = DVAR_GET_INT(gfx_voxel_size);
    DEV_ASSERT(math::IsPowerOfTwo(voxel_texture_size));
    DEV_ASSERT(voxel_texture_size <= 256);

    vec3 world_center = p_scene.GetBound().Center();
    vec3 aabb_size = p_scene.GetBound().Size();
    float world_size = glm::max(aabb_size.x, glm::max(aabb_size.y, aabb_size.z));

    const float max_world_size = DVAR_GET_FLOAT(gfx_vxgi_max_world_size);
    if (world_size > max_world_size) {
        world_center = camera.GetPosition();
        world_size = max_world_size;
    }

    const float texel_size = 1.0f / static_cast<float>(voxel_texture_size);
    const float voxel_size = world_size * texel_size;

    cache.c_worldCenter = world_center;
    cache.c_worldSizeHalf = 0.5f * world_size;
    cache.c_texelSize = texel_size;
    cache.c_voxelSize = voxel_size;

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
    auto find_index = [&](RenderTargetResourceName p_name) -> uint32_t {
        std::shared_ptr<GpuTexture> resource = FindTexture(p_name);
        if (!resource) {
            return 0;
        }

        return static_cast<uint32_t>(resource->GetResidentHandle());
    };

    cache.c_GbufferBaseColorMapIndex = find_index(RESOURCE_GBUFFER_BASE_COLOR);
    cache.c_GbufferPositionMapIndex = find_index(RESOURCE_GBUFFER_POSITION);
    cache.c_GbufferNormalMapIndex = find_index(RESOURCE_GBUFFER_NORMAL);
    cache.c_GbufferMaterialMapIndex = find_index(RESOURCE_GBUFFER_MATERIAL);

    cache.c_GbufferDepthIndex = find_index(RESOURCE_GBUFFER_DEPTH);
    cache.c_PointShadowArrayIndex = find_index(RESOURCE_POINT_SHADOW_CUBE_ARRAY);
    cache.c_ShadowMapIndex = find_index(RESOURCE_SHADOW_MAP);

    cache.c_TextureHighlightSelectIndex = find_index(RESOURCE_HIGHLIGHT_SELECT);
    cache.c_TextureLightingIndex = find_index(RESOURCE_LIGHTING);
}

void GraphicsManager::UpdateEmitters(const Scene& p_scene) {
    // @HACK: skip for now
    if (GetBackend() == Backend::D3D12) {
        return;
    }

    for (auto [id, emitter] : p_scene.m_ParticleEmitterComponents) {
        if (!emitter.particleBuffer) {
            // create buffer
            emitter.counterBuffer = CreateStructuredBuffer({
                .elementSize = sizeof(ParticleCounter),
                .elementCount = 1,
            });
            emitter.deadBuffer = CreateStructuredBuffer({
                .elementSize = sizeof(int),
                .elementCount = MAX_PARTICLE_COUNT,
            });
            emitter.aliveBuffer[0] = CreateStructuredBuffer({
                .elementSize = sizeof(int),
                .elementCount = MAX_PARTICLE_COUNT,
            });
            emitter.aliveBuffer[1] = CreateStructuredBuffer({
                .elementSize = sizeof(int),
                .elementCount = MAX_PARTICLE_COUNT,
            });
            emitter.particleBuffer = CreateStructuredBuffer({
                .elementSize = sizeof(Particle),
                .elementCount = MAX_PARTICLE_COUNT,
            });

            SetPipelineState(PSO_PARTICLE_INIT);

            BindStructuredBuffer(GetGlobalParticleCounterSlot(), emitter.counterBuffer.get());
            BindStructuredBuffer(GetGlobalDeadIndicesSlot(), emitter.deadBuffer.get());
            Dispatch(MAX_PARTICLE_COUNT / PARTICLE_LOCAL_SIZE, 1, 1);
            UnbindStructuredBuffer(GetGlobalParticleCounterSlot());
            UnbindStructuredBuffer(GetGlobalDeadIndicesSlot());
        }

        const uint32_t pre_sim_idx = emitter.GetPreIndex();
        const uint32_t post_sim_idx = emitter.GetPostIndex();
        EmitterConstantBuffer buffer;
        const TransformComponent* transform = p_scene.GetComponent<TransformComponent>(id);
        buffer.c_preSimIdx = pre_sim_idx;
        buffer.c_postSimIdx = post_sim_idx;
        buffer.c_elapsedTime = p_scene.m_elapsedTime;
        buffer.c_lifeSpan = emitter.particleLifeSpan;
        buffer.c_seeds = vec3(Random::Float(), Random::Float(), Random::Float());
        buffer.c_emitterScale = emitter.particleScale;
        buffer.c_emitterPosition = transform->GetTranslation();
        buffer.c_particlesPerFrame = emitter.particlesPerFrame;
        buffer.c_emitterStartingVelocity = emitter.startingVelocity;
        buffer.c_emitterMaxParticleCount = emitter.maxParticleCount;
        buffer.c_emitterHasGravity = emitter.gravity;

        GetCurrentFrame().emitterCache.push_back(buffer);
    }
}

void GraphicsManager::UpdateForceFields(const Scene& p_scene) {
    unused(p_scene);
}

/// @TODO: refactor lights
void GraphicsManager::UpdateLights(const Scene& p_scene) {
    const uint32_t light_count = glm::min<uint32_t>((uint32_t)p_scene.GetCount<LightComponent>(), MAX_LIGHT_COUNT);

    auto& cache = GetCurrentFrame().perFrameCache;
    cache.c_lightCount = light_count;

    auto& point_shadow_cache = GetCurrentFrame().pointShadowCache;

    int idx = 0;
    for (auto [light_entity, light_component] : p_scene.m_LightComponents) {
        const TransformComponent* light_transform = p_scene.GetComponent<TransformComponent>(light_entity);
        const MaterialComponent* material = p_scene.GetComponent<MaterialComponent>(light_entity);

        DEV_ASSERT(light_transform && material);

        // SHOULD BE THIS INDEX
        Light& light = cache.c_lights[idx];
        bool cast_shadow = light_component.CastShadow();
        light.cast_shadow = cast_shadow;
        light.type = light_component.GetType();
        light.color = material->baseColor;
        light.color *= material->emissive;
        switch (light_component.GetType()) {
            case LIGHT_TYPE_INFINITE: {
                mat4 light_local_matrix = light_transform->GetLocalMatrix();
                vec3 light_dir = glm::normalize(light_local_matrix * vec4(0, 0, 1, 0));
                light.cast_shadow = cast_shadow;
                light.position = light_dir;

                const AABB& world_bound = p_scene.GetBound();
                const vec3 center = world_bound.Center();
                const vec3 extents = world_bound.Size();
                const float size = 0.7f * glm::max(extents.x, glm::max(extents.y, extents.z));

                light.view_matrix = glm::lookAt(center + light_dir * size, center, vec3(0, 1, 0));

                if (GetBackend() == Backend::OPENGL) {
                    light.projection_matrix = BuildOpenGlOrthoRH(-size, size, -size, size, -size, 3.0f * size);
                } else {
                    light.projection_matrix = BuildOrthoRH(-size, size, -size, size, -size, 3.0f * size);
                }

                PerPassConstantBuffer pass_constant;
                // @TODO: Build correct matrices
                pass_constant.c_projectionMatrix = light.projection_matrix;
                pass_constant.c_viewMatrix = light.view_matrix;
                m_shadowPasses[0].pass_idx = static_cast<int>(GetCurrentFrame().passCache.size());
                GetCurrentFrame().passCache.emplace_back(pass_constant);

                // Frustum light_frustum(light_space_matrix);
                FillPass(
                    p_scene,
                    m_shadowPasses[0],
                    [](const ObjectComponent& p_object) {
                        return p_object.flags & ObjectComponent::CAST_SHADOW;
                    },
                    [&](const AABB& p_aabb) {
                        unused(p_aabb);
                        // return light_frustum.intersects(aabb);
                        return true;
                    });
            } break;
            case LIGHT_TYPE_POINT: {
                const int shadow_map_index = light_component.GetShadowMapIndex();
                // @TODO: there's a bug in shadow map allocation
                light.atten_constant = light_component.m_atten.constant;
                light.atten_linear = light_component.m_atten.linear;
                light.atten_quadratic = light_component.m_atten.quadratic;
                light.position = light_component.GetPosition();
                light.cast_shadow = cast_shadow;
                light.max_distance = light_component.GetMaxDistance();
                if (cast_shadow && shadow_map_index != INVALID_POINT_SHADOW_HANDLE) {
                    light.shadow_map_index = shadow_map_index;

                    auto pass = std::make_unique<PassContext>();
                    FillPass(
                        p_scene,
                        *pass.get(),
                        [](const ObjectComponent& object) {
                            return object.flags & ObjectComponent::CAST_SHADOW;
                        },
                        [&](const AABB& aabb) {
                            unused(aabb);
                            return true;
                        });

                    DEV_ASSERT_INDEX(shadow_map_index, MAX_POINT_LIGHT_SHADOW_COUNT);
                    const auto& light_matrices = light_component.GetMatrices();
                    for (int face_id = 0; face_id < 6; ++face_id) {
                        const uint32_t slot = shadow_map_index * 6 + face_id;
                        point_shadow_cache[slot].c_pointLightMatrix = light_matrices[face_id];
                        point_shadow_cache[slot].c_pointLightPosition = light_component.GetPosition();
                        point_shadow_cache[slot].c_pointLightFar = light_component.GetMaxDistance();
                    }

                    m_pointShadowPasses[shadow_map_index] = std::move(pass);
                } else {
                    light.shadow_map_index = -1;
                }
            } break;
            case LIGHT_TYPE_AREA: {
                mat4 transform = light_transform->GetWorldMatrix();
                constexpr float s = 0.5f;
                light.points[0] = transform * vec4(-s, +s, 0.0f, 1.0f);
                light.points[1] = transform * vec4(-s, -s, 0.0f, 1.0f);
                light.points[2] = transform * vec4(+s, -s, 0.0f, 1.0f);
                light.points[3] = transform * vec4(+s, +s, 0.0f, 1.0f);
            } break;
            default:
                CRASH_NOW();
                break;
        }
        ++idx;
    }
}

void GraphicsManager::UpdateVoxelPass(const Scene& p_scene) {
    FillPass(
        p_scene,
        m_voxelPass,
        [](const ObjectComponent& object) {
            return object.flags & ObjectComponent::RENDERABLE;
        },
        [&](const AABB& aabb) {
            unused(aabb);
            // return scene->get_bound().intersects(aabb);
            return true;
        });
}

void GraphicsManager::UpdateMainPass(const Scene& p_scene) {
    const Camera& camera = *p_scene.m_camera;
    Frustum camera_frustum(camera.GetProjectionViewMatrix());

    // main pass
    PerPassConstantBuffer pass_constant;
    pass_constant.c_viewMatrix = camera.GetViewMatrix();

    const float fovy = camera.GetFovy().ToRad();
    const float aspect = camera.GetAspect();
    if (GetBackend() == Backend::OPENGL) {
        pass_constant.c_projectionMatrix = BuildOpenGlPerspectiveRH(fovy, aspect, camera.GetNear(), camera.GetFar());
    } else {
        pass_constant.c_projectionMatrix = BuildPerspectiveRH(fovy, aspect, camera.GetNear(), camera.GetFar());
    }

    m_mainPass.pass_idx = static_cast<int>(GetCurrentFrame().passCache.size());
    GetCurrentFrame().passCache.emplace_back(pass_constant);

    FillPass(
        p_scene,
        m_mainPass,
        [](const ObjectComponent& object) {
            return object.flags & ObjectComponent::RENDERABLE;
        },
        [&](const AABB& aabb) {
            return camera_frustum.Intersects(aabb);
        });
}

void GraphicsManager::FillPass(const Scene& p_scene, PassContext& p_pass, FilterObjectFunc1 p_filter1, FilterObjectFunc2 p_filter2) {
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

        const mat4& world_matrix = transform.GetWorldMatrix();
        AABB aabb = mesh.localBound;
        aabb.ApplyMatrix(world_matrix);
        if (!p_filter2(aabb)) {
            continue;
        }

        PerBatchConstantBuffer batch_buffer;
        batch_buffer.c_worldMatrix = world_matrix;
        batch_buffer.c_hasAnimation = mesh.armatureId.IsValid();

        BatchContext draw;
        draw.flags = 0;
        if (entity == p_scene.m_selected) {
            draw.flags |= STENCIL_FLAG_SELECTED;
        }

        draw.batch_idx = GetCurrentFrame().batchCache.FindOrAdd(entity, batch_buffer);
        if (mesh.armatureId.IsValid()) {
            auto& armature = *p_scene.GetComponent<ArmatureComponent>(mesh.armatureId);
            DEV_ASSERT(armature.boneTransforms.size() <= MAX_BONE_COUNT);

            BoneConstantBuffer bone;
            memcpy(bone.c_bones, armature.boneTransforms.data(), sizeof(mat4) * armature.boneTransforms.size());

            // @TODO: better memory usage
            draw.bone_idx = GetCurrentFrame().boneCache.FindOrAdd(mesh.armatureId, bone);
        } else {
            draw.bone_idx = -1;
        }
        DEV_ASSERT(mesh.gpuResource);
        draw.mesh_data = (MeshBuffers*)mesh.gpuResource;

        for (const auto& subset : mesh.subsets) {
            aabb = subset.local_bound;
            aabb.ApplyMatrix(world_matrix);
            if (!p_filter2(aabb)) {
                continue;
            }

            const MaterialComponent* material = p_scene.GetComponent<MaterialComponent>(subset.material_id);
            MaterialConstantBuffer material_buffer;
            FillMaterialConstantBuffer(material, material_buffer);

            DrawContext sub_mesh;
            sub_mesh.index_count = subset.index_count;
            sub_mesh.index_offset = subset.index_offset;
            sub_mesh.material_idx = GetCurrentFrame().materialCache.FindOrAdd(subset.material_id, material_buffer);

            draw.subsets.emplace_back(std::move(sub_mesh));
        }

        p_pass.draws.emplace_back(std::move(draw));
    }
}

}  // namespace my
