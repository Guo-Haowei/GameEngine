#include "graphics_manager.h"

#include "core/base/random.h"
#include "core/debugger/profiler.h"
#include "core/math/frustum.h"
#include "core/math/matrix_transform.h"
#if USING(PLATFORM_WINDOWS)
#include "drivers/d3d11/d3d11_graphics_manager.h"
#include "drivers/d3d12/d3d12_graphics_manager.h"
#include "drivers/vk/vulkan_graphics_manager.h"
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
#ifdef GetMessage
#undef GetMessage
#endif

namespace my::pt {

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::mat3;
using glm::mat4;
using glm::uvec2;
using glm::uvec3;
using glm::uvec4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

struct Geometry {
    enum class Kind {
        Invalid,
        Triangle,
        Sphere,
        Count
    };

    Vector3f A;
    Kind kind;
    Vector3f B;
    float radius;
    Vector3f C;
    int materialId;
    Vector2f uv1;
    Vector2f uv2;
    Vector3f normal1;
    float uv3x;
    Vector3f normal2;
    float uv3y;
    Vector3f normal3;
    float hasAlbedoMap = 0.0f;

    Geometry();
    Geometry(const Vector3f& A, const Vector3f& B, const Vector3f& C, int material);
    Geometry(const Vector3f& center, float radius, int material);
    Vector3f Centroid() const;
    void CalcNormal();
};

using GeometryList = std::vector<Geometry>;
static_assert(sizeof(Geometry) % sizeof(Vector4f) == 0);

struct Box3 {
    static constexpr float minSpan = 0.001f;

    Vector3f min;
    Vector3f max;

    Box3();
    Box3(const Vector3f& min, const Vector3f& max);
    Box3(const Box3& box1, const Box3& box2);

    Vector3f Center() const;
    float SurfaceArea() const;
    bool IsValid() const;
    void Expand(const Vector3f& point);
    void Expand(const Box3& box);

    void MakeValid();

    static Box3 FromGeometry(const Geometry& geom);
    static Box3 FromGeometries(const GeometryList& geoms);
    static Box3 FromGeometriesCentroid(const GeometryList& geoms);
};

// @TODO: remove
struct SceneCamera {
    static constexpr float DEFAULT_FOV = 60.0f;

    std::string name;
    float fov;
    Vector3f eye;
    Vector3f lookAt;
    Vector3f up;

    SceneCamera()
        : fov(DEFAULT_FOV), eye(Vector4f(0, 0, 0, 1)), lookAt(Vector3f(0)), up(Vector3f(0, 1, 0)) {}
};

struct SceneMat {
    std::string name;
    Vector3f albedo;
    Vector3f emissive;
    float reflect;
    float roughness;

    SceneMat()
        : albedo(Vector3f(1)), emissive(Vector3f(0)), reflect(0.f), roughness(1.f) {}
};

struct SceneGeometry {
    enum class Kind {
        Invalid,
        Sphere,
        Quad,
        Cube,
        Mesh,
    };

    SceneGeometry()
        : kind(Kind::Invalid), materidId(-1), translate(Vector3f(0)), euler(Vector3f(0)), scale(Vector3f(1)) {}

    std::string name;
    std::string path;
    Kind kind;
    int materidId;
    Vector3f translate;
    Vector3f euler;
    Vector3f scale;
};

struct GpuBvh {
    Vector3f min;
    int missIdx;
    Vector3f max;
    int hitIdx;

    int leaf;
    int geomIdx;
    int padding[2];

    GpuBvh();
};

struct GpuMaterial {
    Vector3f albedo;
    float reflect;
    Vector3f emissive;
    float roughness;
    float albedoMapLevel;
    int padding[3];
};

struct GpuScene {
    std::vector<GpuMaterial> materials;
    std::vector<Geometry> geometries;
    std::vector<GpuBvh> bvhs;

    int height;
    Box3 bbox;
};

void ConstructScene(const Scene& inScene, GpuScene& outScene);
//////////////////////////////////
}  // namespace my::pt

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
    p_buffer.buffer = *GraphicsManager::GetSingleton().CreateConstantBuffer(buffer_desc);
}

auto GraphicsManager::Initialize() -> Result<void> {
    m_enableValidationLayer = DVAR_GET_BOOL(gfx_gpu_validation);

    const int num_frames = (GetBackend() == Backend::D3D12) ? NUM_FRAMES_IN_FLIGHT : 1;
    m_frameContexts.resize(num_frames);
    for (int i = 0; i < num_frames; ++i) {
        m_frameContexts[i] = CreateFrameContext();
    }
    if (auto res = InitializeImpl(); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = SelectRenderGraph(); !res) {
        return HBN_ERROR(res.error());
    }

    for (int i = 0; i < num_frames; ++i) {
        FrameContext& frame_context = *m_frameContexts[i].get();
        frame_context.batchCb = *::my::CreateUniformCheckSize<PerBatchConstantBuffer>(*this, 4096 * 16);
        frame_context.passCb = *::my::CreateUniformCheckSize<PerPassConstantBuffer>(*this, 32);
        frame_context.materialCb = *::my::CreateUniformCheckSize<MaterialConstantBuffer>(*this, 2048 * 16);
        frame_context.boneCb = *::my::CreateUniformCheckSize<BoneConstantBuffer>(*this, 16);
        frame_context.emitterCb = *::my::CreateUniformCheckSize<EmitterConstantBuffer>(*this, 32);
        frame_context.pointShadowCb = *::my::CreateUniformCheckSize<PointShadowConstantBuffer>(*this, 6 * MAX_POINT_LIGHT_SHADOW_COUNT);
        frame_context.perFrameCb = *::my::CreateUniformCheckSize<PerFrameConstantBuffer>(*this, 1);
    }

    // @TODO: refactor
    CreateUniformBuffer<PerSceneConstantBuffer>(g_constantCache);
    CreateUniformBuffer<DebugDrawConstantBuffer>(g_debug_draw_cache);
    CreateUniformBuffer<EnvConstantBuffer>(g_env_cache);

    DEV_ASSERT(m_pipelineStateManager);

    if (auto res = m_pipelineStateManager->Initialize(); !res) {
        return HBN_ERROR(res.error());
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
    SRV_DEFINES
#undef SRV

    m_initialized = true;
    return Result<void>();
}

void GraphicsManager::EventReceived(std::shared_ptr<IEvent> p_event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(p_event.get()); e) {
        // @TODO: refactor
        if (m_renderGraphName == RenderGraphName::PATHTRACER) {
            pt::GpuScene gpuScene;
            pt::ConstructScene(*e->GetScene(), gpuScene);

            m_geometryBuffer = *CreateStructuredBuffer({
                .slot = GetGlobalGeometriesSlot(),
                .elementSize = sizeof(pt::Geometry),
                .elementCount = (uint32_t)gpuScene.geometries.size(),
                .initialData = gpuScene.geometries.data(),
            });
            m_bvhBuffer = *CreateStructuredBuffer({
                .slot = GetGlobalBvhsSlot(),
                .elementSize = sizeof(pt::GpuBvh),
                .elementCount = (uint32_t)gpuScene.bvhs.size(),
                .initialData = gpuScene.bvhs.data(),
            });
        }

        OnSceneChange(*e->GetScene());
    }
    if (ResizeEvent* e = dynamic_cast<ResizeEvent*>(p_event.get()); e) {
        OnWindowResize(e->GetWidth(), e->GetHeight());
    }
}

auto GraphicsManager::Create() -> Result<std::shared_ptr<GraphicsManager>> {
    const std::string& backend = DVAR_GET_STRING(gfx_backend);

    auto select_renderer = [&]() -> std::shared_ptr<GraphicsManager> {
        auto create_empty_renderer = []() -> std::shared_ptr<GraphicsManager> {
            return std::make_shared<EmptyGraphicsManager>("EmptyGraphicsManager", Backend::EMPTY, 1);
        };

        auto create_d3d11_renderer = []() -> std::shared_ptr<GraphicsManager> {
#if USING(PLATFORM_WINDOWS)
            return std::make_shared<D3d11GraphicsManager>();
#else
            return nullptr;
#endif
        };
        auto create_d3d12_renderer = []() -> std::shared_ptr<GraphicsManager> {
#if USING(PLATFORM_WINDOWS)
            return std::make_shared<D3d12GraphicsManager>();
#else
            return nullptr;
#endif
        };
        auto create_vulkan_renderer = []() -> std::shared_ptr<GraphicsManager> {
#if USING(PLATFORM_WINDOWS)
            return std::make_shared<VulkanGraphicsManager>();
#else
            return nullptr;
#endif
        };
        auto create_opengl_renderer = []() -> std::shared_ptr<GraphicsManager> {
#if USING(PLATFORM_WINDOWS)
            return std::make_shared<OpenGlGraphicsManager>();
#else
            return nullptr;
#endif
        };
        auto create_metal_renderer = []() -> std::shared_ptr<GraphicsManager> {
#if USING(PLATFORM_APPLE)
            return std::make_shared<EmptyGraphicsManager>("Emtpy", Backend::EMPTY, 1);
#else
            return nullptr;
#endif
        };

#define BACKEND_DECLARE(ENUM, DISPLAY, DVAR) \
    if (backend == #DVAR) { return create_##DVAR##_renderer(); }
        BACKEND_LIST
#undef BACKEND_DECLARE
        return nullptr;
    };

    auto result = select_renderer();
    if (!result) {
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "backend '{}' not supported", backend);
    }
    return result;
}

void GraphicsManager::SetPipelineState(PipelineStateName p_name) {
    SetPipelineStateImpl(p_name);
}

void GraphicsManager::RequestTexture(ImageHandle* p_handle, OnTextureLoadFunc p_func) {
    m_loadedImages.push(ImageTask{ p_handle, p_func });
}

void GraphicsManager::Update(Scene& p_scene) {
    OPTICK_EVENT();

    [[maybe_unused]] const Backend backend = GetBackend();

    Cleanup();

    UpdateConstants(p_scene);
    UpdateForceFields(p_scene);
    UpdateLights(p_scene);
    UpdateVoxelPass(p_scene);
    UpdateMainPass(p_scene);
    UpdateBloomConstants();

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

        UpdateEmitters(p_scene);

        auto& frame = GetCurrentFrame();
        UpdateConstantBuffer(frame.batchCb.get(), frame.batchCache.buffer);
        UpdateConstantBuffer(frame.materialCb.get(), frame.materialCache.buffer);
        UpdateConstantBuffer(frame.boneCb.get(), frame.boneCache.buffer);
        UpdateConstantBuffer(frame.passCb.get(), frame.passCache);
        UpdateConstantBuffer(frame.emitterCb.get(), frame.emitterCache);
        UpdateConstantBuffer<PointShadowConstantBuffer, 6 * MAX_POINT_LIGHT_SHADOW_COUNT>(frame.pointShadowCb.get(), frame.pointShadowCache);
        UpdateConstantBuffer(frame.perFrameCb.get(), &frame.perFrameCache, sizeof(PerFrameConstantBuffer));
        BindConstantBufferSlot<PerFrameConstantBuffer>(frame.perFrameCb.get(), 0);

        // @HACK
        if (backend != Backend::VULKAN) {
            m_renderGraph.Execute(*this);
        }

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
    for (auto& texture : p_draw_pass->outSrvs) {
        if (texture->slot >= 0) {
            UnbindTexture(Dimension::TEXTURE_2D, texture->slot);
            // RT_DEBUG("  -- unbound resource '{}'({})", RenderTargetResourceNameToString(it->desc.name), it->slot);
        }
    }

    for (auto& transition : p_draw_pass->desc.transitions) {
        if (transition.beginPassFunc) {
            transition.beginPassFunc(this, transition.resource.get(), transition.slot);
        }
    }
}

void GraphicsManager::EndDrawPass(const DrawPass* p_draw_pass) {
    UnsetRenderTarget();
    for (auto& texture : p_draw_pass->outSrvs) {
        if (texture->slot >= 0) {
            BindTexture(Dimension::TEXTURE_2D, texture->GetHandle(), texture->slot);
            // RT_DEBUG("  -- bound resource '{}'({})", RenderTargetResourceNameToString(it->desc.name), it->slot);
        }
    }

    for (auto& transition : p_draw_pass->desc.transitions) {
        if (transition.endPassFunc) {
            transition.endPassFunc(this, transition.resource.get(), transition.slot);
        }
    }
}

auto GraphicsManager::SelectRenderGraph() -> Result<void> {
    std::string method(DVAR_GET_STRING(gfx_render_graph));
    static const std::map<std::string, RenderGraphName> lookup = {
        { "dummy", RenderGraphName::DUMMY },
        { "default", RenderGraphName::DEFAULT },
        { "experimental", RenderGraphName::EXPERIMENTAL },
        { "pathtracer", RenderGraphName::PATHTRACER },
    };

    if (GetBackend() == Backend::METAL) {
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE);
    }

    if (!method.empty()) {
        auto it = lookup.find(method);
        if (it == lookup.end()) {
            return HBN_ERROR(ErrorCode::ERR_INVALID_PARAMETER, "unknown render graph '{}'", method);
        } else {
            m_renderGraphName = it->second;
        }
    }

    switch (GetBackend()) {
        case Backend::VULKAN:
        case Backend::EMPTY:
        case Backend::METAL:
            m_renderGraphName = RenderGraphName::DUMMY;
            break;
        default:
            break;
    }

    // force to default
    if (m_backend != Backend::OPENGL && m_renderGraphName == RenderGraphName::EXPERIMENTAL) {
        LOG_WARN("'experimental' not supported, fall back to 'default'");
        m_renderGraphName = RenderGraphName::DEFAULT;
    }

    switch (m_renderGraphName) {
        case RenderGraphName::DUMMY:
            rg::RenderPassCreator::CreateDummy(m_renderGraph);
            break;
        case RenderGraphName::EXPERIMENTAL:
            rg::RenderPassCreator::CreateExperimental(m_renderGraph);
            break;
        case RenderGraphName::DEFAULT:
            rg::RenderPassCreator::CreateDefault(m_renderGraph);
            break;
        case RenderGraphName::PATHTRACER:
            rg::RenderPassCreator::CreatePathTracer(m_renderGraph);
            break;
        default:
            DEV_ASSERT(0 && "Should not reach here");
            return HBN_ERROR(ErrorCode::ERR_INVALID_PARAMETER, "unknown render graph '{}'", method);
    }

    return Result<void>();
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
            texture = FindTexture(RESOURCE_GBUFFER_BASE_COLOR).get();
            break;
        case RenderGraphName::EXPERIMENTAL:
            texture = FindTexture(RESOURCE_FINAL).get();
            break;
        case RenderGraphName::DEFAULT:
            texture = FindTexture(RESOURCE_TONE).get();
            break;
        case RenderGraphName::PATHTRACER:
            // texture = FindTexture(RESOURCE_GBUFFER_BASE_COLOR).get();
            texture = FindTexture(RESOURCE_PATH_TRACER).get();
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

    Vector3f world_center = p_scene.GetBound().Center();
    Vector3f aabb_size = p_scene.GetBound().Size();
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
    cache.c_cameraForward = camera.GetFront();
    // cache.c_tileOffset;
    cache.c_cameraRight = camera.GetRight();
    // @TODO: refactor
    static int s_frameIndex = 0;
    cache.c_cameraUp = glm::cross(cache.c_cameraForward, cache.c_cameraRight);
    cache.c_frameIndex = s_frameIndex++;
    cache.c_cameraFov = camera.GetFovy().GetDegree();

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
    for (auto [id, emitter] : p_scene.m_ParticleEmitterComponents) {
        if (!emitter.particleBuffer) {
            // create buffer
            emitter.counterBuffer = *CreateStructuredBuffer({
                .elementSize = sizeof(ParticleCounter),
                .elementCount = 1,
            });
            emitter.deadBuffer = *CreateStructuredBuffer({
                .elementSize = sizeof(int),
                .elementCount = MAX_PARTICLE_COUNT,
            });
            emitter.aliveBuffer[0] = *CreateStructuredBuffer({
                .elementSize = sizeof(int),
                .elementCount = MAX_PARTICLE_COUNT,
            });
            emitter.aliveBuffer[1] = *CreateStructuredBuffer({
                .elementSize = sizeof(int),
                .elementCount = MAX_PARTICLE_COUNT,
            });
            emitter.particleBuffer = *CreateStructuredBuffer({
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
        buffer.c_seeds = Vector3f(Random::Float(), Random::Float(), Random::Float());
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
                Matrix4x4f light_local_matrix = light_transform->GetLocalMatrix();
                Vector3f light_dir = glm::normalize(light_local_matrix * Vector4f(0, 0, 1, 0));
                light.cast_shadow = cast_shadow;
                light.position = light_dir;

                const AABB& world_bound = p_scene.GetBound();
                const Vector3f center = world_bound.Center();
                const Vector3f extents = world_bound.Size();
                const float size = 0.7f * glm::max(extents.x, glm::max(extents.y, extents.z));

                light.view_matrix = glm::lookAt(center + light_dir * size, center, Vector3f(0, 1, 0));

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

                Frustum light_frustum(light.projection_matrix * light.view_matrix);
                FillPass(
                    p_scene,
                    m_shadowPasses[0],
                    [](const ObjectComponent& p_object) {
                        return p_object.flags & ObjectComponent::CAST_SHADOW;
                    },
                    [&](const AABB& p_aabb) {
                        return light_frustum.Intersects(p_aabb);
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

                    Vector3f radiance(light.max_distance);
                    AABB aabb = AABB::FromCenterSize(light.position, radiance);
                    auto pass = std::make_unique<PassContext>();
                    FillPass(
                        p_scene,
                        *pass.get(),
                        [](const ObjectComponent& p_object) {
                            return p_object.flags & ObjectComponent::CAST_SHADOW;
                        },
                        [&](const AABB& p_aabb) {
                            return p_aabb.Intersects(aabb);
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

void GraphicsManager::UpdateVoxelPass(const Scene& p_scene) {
    if (!DVAR_GET_BOOL(gfx_enable_vxgi)) {
        return;
    }
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

void GraphicsManager::UpdateBloomConstants() {
    auto image = FindTexture(RESOURCE_BLOOM_0).get();
    if (!image) {
        return;
    }
    constexpr int count = BLOOM_MIP_CHAIN_MAX * 2 - 1;
    auto& frame = GetCurrentFrame();
    if (frame.batchCache.buffer.size() < count) {
        frame.batchCache.buffer.resize(count);
    }

    int offset = 0;
    frame.batchCache.buffer[offset++].c_BloomOutputImageIndex = (uint)image->GetUavHandle();

    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX - 1; ++i) {
        auto input = FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i + 1));

        frame.batchCache.buffer[i + offset].c_BloomInputTextureIndex = (uint)input->GetResidentHandle();
        frame.batchCache.buffer[i + offset].c_BloomOutputImageIndex = (uint)output->GetUavHandle();
    }

    offset += BLOOM_MIP_CHAIN_MAX - 1;

    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto input = FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));

        frame.batchCache.buffer[i - 1 + offset].c_BloomInputTextureIndex = (uint)input->GetResidentHandle();
        frame.batchCache.buffer[i - 1 + offset].c_BloomOutputImageIndex = (uint)output->GetUavHandle();
    }
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

        const Matrix4x4f& world_matrix = transform.GetWorldMatrix();
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
            memcpy(bone.c_bones, armature.boneTransforms.data(), sizeof(Matrix4x4f) * armature.boneTransforms.size());

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

namespace my::pt {

Geometry::Geometry()
    : kind(Kind::Invalid), materialId(-1) {}

Geometry::Geometry(const Vector3f& A, const Vector3f& B, const Vector3f& C, int material)
    : A(A), B(B), C(C), materialId(material) {
    kind = Kind::Triangle;
    CalcNormal();
}

Geometry::Geometry(const Vector3f& center, float radius, int material)
    : A(center), materialId(material) {
    // TODO: refactor
    kind = Kind::Sphere;
    this->radius = glm::max(0.01f, glm::abs(radius));
}

void Geometry::CalcNormal() {
    using glm::cross;
    using glm::normalize;
    Vector3f BA = normalize(B - A);
    Vector3f CA = normalize(C - A);
    Vector3f norm = normalize(cross(BA, CA));
    normal1 = norm;
    normal2 = norm;
    normal3 = norm;
}

Vector3f Geometry::Centroid() const {
    switch (kind) {
        case Kind::Triangle:
            return (A + B + C) / 3.0f;
        case Kind::Sphere:
            return A;
        default:
            assert(0);
            return Vector3f(0.0f);
    }
}

using GeometryList = std::vector<Geometry>;
static_assert(sizeof(Geometry) % sizeof(Vector4f) == 0);

Box3::Box3()
    : min(std::numeric_limits<float>::infinity()), max(-std::numeric_limits<float>::infinity()) {}

Box3::Box3(const Vector3f& min, const Vector3f& max)
    : min(min), max(max) {}

Box3::Box3(const Box3& box1, const Box3& box2)
    : min(glm::min(box1.min, box2.min)), max(glm::max(box1.max, box2.max)) {}

Vector3f Box3::Center() const {
    return 0.5f * (min + max);
}

void Box3::Expand(const Vector3f& p) {
    min = glm::min(min, p);
    max = glm::max(max, p);
}

void Box3::Expand(const Box3& box) {
    min = glm::min(min, box.min);
    max = glm::max(max, box.max);
}

bool Box3::IsValid() const {
    return min.x < max.y && min.y < max.y && min.z < max.z;
}

void Box3::MakeValid() {
    for (int i = 0; i < 3; ++i) {
        if (min[i] == max[i]) {
            min[i] -= Box3::minSpan;
            max[i] += Box3::minSpan;
        }
    }
}

float Box3::SurfaceArea() const {
    if (!IsValid()) {
        return 0.0f;
    }
    Vector3f span = glm::abs(max - min);
    float result = 2.0f * (span.x * span.y +
                           span.x * span.z +
                           span.y * span.z);
    return result;
}

static Box3 Box3FromSphere(const Geometry& sphere) {
    assert(sphere.kind == Geometry::Kind::Sphere);

    return Box3(sphere.A - Vector3f(sphere.radius), sphere.A + Vector3f(sphere.radius));
}

static Box3 Box3FromTriangle(const Geometry& triangle) {
    assert(triangle.kind == Geometry::Kind::Triangle);

    Box3 ret = Box3(
        glm::min(triangle.C, glm::min(triangle.A, triangle.B)),
        glm::max(triangle.C, glm::max(triangle.A, triangle.B)));

    ret.MakeValid();
    return ret;
}

Box3 Box3::FromGeometry(const Geometry& geom) {
    switch (geom.kind) {
        case Geometry::Kind::Triangle:
            return Box3FromTriangle(geom);
        case Geometry::Kind::Sphere:
            return Box3FromSphere(geom);
        default:
            assert(0);
    }
    return Box3();
}

Box3 Box3::FromGeometries(const GeometryList& geoms) {
    Box3 box;
    for (const Geometry& geom : geoms) {
        box = Box3(box, Box3::FromGeometry(geom));
    }

    box.MakeValid();
    return box;
}

Box3 Box3::FromGeometriesCentroid(const GeometryList& geoms) {
    Box3 box;
    for (const Geometry& geom : geoms) {
        box.Expand(geom.Centroid());
    }

    box.MakeValid();
    return box;
}

using GpuBvhList = std::vector<GpuBvh>;

static_assert(sizeof(GpuBvh) % sizeof(Vector4f) == 0);

class Bvh {
public:
    Bvh() = delete;
    explicit Bvh(GeometryList& geoms, Bvh* parent = nullptr);
    ~Bvh();

    void CreateGpuBvh(GpuBvhList& outBvh, GeometryList& outTriangles);
    inline const Box3& GetBox() const { return m_box; }

private:
    void SplitByAxis(GeometryList& geoms);
    void DiscoverIdx();

    Box3 m_box;
    Geometry m_geom;
    Bvh* m_left;
    Bvh* m_right;
    bool m_leaf;

    const int m_idx;
    Bvh* const m_parent;

    int m_hitIdx;
    int m_missIdx;
};

GpuBvh::GpuBvh()
    : missIdx(-1), hitIdx(-1), leaf(0), geomIdx(-1) {
    padding[0] = 0;
    padding[1] = 0;
}

static int genIdx() {
    static int idx = 0;
    return idx++;
}

static int DominantAxis(const Box3& box) {
    const Vector3f span = box.max - box.min;
    int axis = 0;
    if (span[axis] < span.y) {
        axis = 1;
    }
    if (span[axis] < span.z) {
        axis = 2;
    }

    return axis;
}

void Bvh::SplitByAxis(GeometryList& geoms) {
    class Sorter {
    public:
        bool operator()(const Geometry& geom1, const Geometry& geom2) {
            Box3 aabb1 = Box3::FromGeometry(geom1);
            Box3 aabb2 = Box3::FromGeometry(geom2);
            Vector3f center1 = aabb1.Center();
            Vector3f center2 = aabb2.Center();
            return center1[axis] < center2[axis];
        }

        Sorter(int axis)
            : axis(axis) {
            assert(axis < 3);
        }

    private:
        const unsigned int axis;
    };

    Sorter sorter(DominantAxis(m_box));

    std::sort(geoms.begin(), geoms.end(), sorter);
    const size_t mid = geoms.size() / 2;
    GeometryList leftPartition(geoms.begin(), geoms.begin() + mid);
    GeometryList rightPartition(geoms.begin() + mid, geoms.end());

    m_left = new Bvh(leftPartition, this);
    m_right = new Bvh(rightPartition, this);
    m_leaf = false;
}

struct SceneStats {
    int height;
    int geomCnt;
    int bboxCnt;
};

extern SceneStats g_SceneStats;

Bvh::Bvh(GeometryList& geometries, Bvh* parent)
    : m_idx(genIdx()), m_parent(parent) {
    m_left = nullptr;
    m_right = nullptr;
    m_leaf = false;
    m_hitIdx = -1;
    m_missIdx = -1;

    const size_t nGeoms = geometries.size();

    assert(nGeoms);

    if (nGeoms == 1) {
        m_geom = geometries.front();
        m_leaf = true;
        m_box = Box3::FromGeometry(m_geom);
        return;
    }

    m_box = Box3::FromGeometries(geometries);
    const float boxSurfaceArea = m_box.SurfaceArea();

    if (nGeoms <= 4 || boxSurfaceArea == 0.0f) {
        SplitByAxis(geometries);
        return;
    }

    constexpr int nBuckets = 12;
    struct BucketInfo {
        int count = 0;
        Box3 box;
    };
    BucketInfo buckets[nBuckets];

    std::vector<Vector3f> centroids(nGeoms);

    Box3 centroidBox;
    for (size_t i = 0; i < nGeoms; ++i) {
        centroids[i] = geometries.at(i).Centroid();
        centroidBox.Expand(centroids[i]);
    }

    const int axis = DominantAxis(centroidBox);
    const float tmin = centroidBox.min[axis];
    const float tmax = centroidBox.max[axis];

    for (size_t i = 0; i < nGeoms; ++i) {
        float tmp = ((centroids.at(i)[axis] - tmin) * nBuckets) / (tmax - tmin);
        int slot = static_cast<int>(tmp);
        slot = glm::clamp(slot, 0, nBuckets - 1);
        BucketInfo& bucket = buckets[slot];
        ++bucket.count;
        bucket.box.Expand(Box3::FromGeometry(geometries.at(i)));
    }

    float costs[nBuckets - 1];
    for (int i = 0; i < nBuckets - 1; ++i) {
        Box3 b0, b1;
        int count0 = 0, count1 = 0;
        for (int j = 0; j <= i; ++j) {
            b0.Expand(buckets[j].box);
            count0 += buckets[j].count;
        }
        for (int j = i + 1; j < nBuckets; ++j) {
            b1.Expand(buckets[j].box);
            count1 += buckets[j].count;
        }

        constexpr float travCost = 0.125f;
        // constexpr float intersectCost = 1.f;
        costs[i] = travCost + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / boxSurfaceArea;
    }

    int splitIndex = 0;
    float minCost = costs[splitIndex];
    for (int i = 0; i < nBuckets - 1; ++i) {
        // printf("cost of split after bucket %d is %f\n", i, costs[i]);
        if (costs[i] < minCost) {
            splitIndex = i;
            minCost = costs[splitIndex];
        }
    }

    // printf("split index is %d\n", splitIndex);

    GeometryList leftPartition;
    GeometryList rightPartition;

    for (const Geometry& geom : geometries) {
        const Vector3f t = geom.Centroid();
        float tmp = (t[axis] - tmin) / (tmax - tmin);
        tmp *= nBuckets;
        int slot = static_cast<int>(tmp);
        slot = glm::clamp(slot, 0, nBuckets - 1);
        if (slot <= splitIndex) {
            leftPartition.push_back(geom);
        } else {
            rightPartition.push_back(geom);
        }
    }

    // printf("left has %llu\n", leftPartition.size());
    // printf("right has %llu\n", rightPartition.size());

    m_left = new Bvh(leftPartition, this);
    m_right = new Bvh(rightPartition, this);

    m_leaf = false;
}

Bvh::~Bvh() {
    // TODO: free memory
}

void Bvh::DiscoverIdx() {
    // hit link (find right link)
    m_missIdx = -1;
    for (const Bvh* cursor = m_parent; cursor; cursor = cursor->m_parent) {
        if (cursor->m_right && cursor->m_right->m_idx > m_idx) {
            m_missIdx = cursor->m_right->m_idx;
            break;
        }
    }

    m_hitIdx = m_left ? m_left->m_idx : m_missIdx;
}

void Bvh::CreateGpuBvh(GpuBvhList& outBvh, GeometryList& outGeometries) {
    DiscoverIdx();

    GpuBvh gpuBvh;
    gpuBvh.min = m_box.min;
    gpuBvh.max = m_box.max;
    gpuBvh.hitIdx = m_hitIdx;
    gpuBvh.missIdx = m_missIdx;
    gpuBvh.leaf = m_leaf;
    gpuBvh.geomIdx = -1;
    if (m_leaf) {
        gpuBvh.geomIdx = static_cast<int>(outGeometries.size());
        outGeometries.push_back(m_geom);
    }

    outBvh.push_back(gpuBvh);
    if (m_left) {
        m_left->CreateGpuBvh(outBvh, outGeometries);
    }
    if (m_right) {
        m_right->CreateGpuBvh(outBvh, outGeometries);
    }
}

static void AddObject(const MeshComponent& p_mesh, const TransformComponent& p_transform, GeometryList& outGeoms) {
    // Loop over shapes
    Matrix4x4f transform = p_transform.GetWorldMatrix();

    for (size_t index = 0; index < p_mesh.indices.size(); index += 3) {
        const uint32_t index0 = p_mesh.indices[index];
        const uint32_t index1 = p_mesh.indices[index + 1];
        const uint32_t index2 = p_mesh.indices[index + 2];

        Vector3f points[3] = {
            transform * Vector4f(p_mesh.positions[index0], 1.0f),
            transform * Vector4f(p_mesh.positions[index1], 1.0f),
            transform * Vector4f(p_mesh.positions[index2], 1.0f),
        };

        Vector3f normals[3] = {
            transform * Vector4f(p_mesh.normals[index0], 0.0f),
            transform * Vector4f(p_mesh.normals[index1], 0.0f),
            transform * Vector4f(p_mesh.normals[index2], 0.0f),
        };

        // per-face material
        Geometry geom(points[0], points[1], points[2], 0);
        // geom.uv1 = uvs[0];
        // geom.uv2 = uvs[1];
        // geom.uv3x = uvs[2].x;
        // geom.uv3y = uvs[2].y;
        geom.normal1 = normals[0];
        geom.normal2 = normals[1];
        geom.normal3 = normals[2];
        geom.kind = pt::Geometry::Kind::Triangle;
        outGeoms.push_back(geom);
    }
}

void ConstructScene(const Scene& p_scene, GpuScene& outScene) {
    /// materials
    outScene.materials.clear();
    // for (const SceneMat& mat : inScene.materials) {
    //     GpuMaterial gpuMat;
    //     gpuMat.albedo = mat.albedo;
    //     gpuMat.emissive = mat.emissive;
    //     gpuMat.reflect = mat.reflect;
    //     gpuMat.roughness = mat.roughness;
    //     gpuMat.albedoMapLevel = 0.0f;
    //     outScene.materials.push_back(gpuMat);
    // }

    /// objects
    outScene.geometries.clear();
    GeometryList tmpGpuObjects;
    for (auto [entity, object] : p_scene.m_ObjectComponents) {
        // ObjectComponent
        auto transform = p_scene.GetComponent<TransformComponent>(entity);
        auto mesh = p_scene.GetComponent<MeshComponent>(object.meshId);
        DEV_ASSERT(transform);
        DEV_ASSERT(mesh);
        AddObject(*mesh, *transform, tmpGpuObjects);
    }

    /// construct bvh
    Bvh root(tmpGpuObjects);
    root.CreateGpuBvh(outScene.bvhs, outScene.geometries);

    outScene.bbox = root.GetBox();

    /// adjust bbox
    for (GpuBvh& bvh : outScene.bvhs) {
        for (int i = 0; i < 3; ++i) {
            if (glm::abs(bvh.max[i] - bvh.min[i]) < 0.01f) {
                bvh.min[i] -= Box3::minSpan;
                bvh.max[i] += Box3::minSpan;
            }
        }
    }
}

}  // namespace my::pt
