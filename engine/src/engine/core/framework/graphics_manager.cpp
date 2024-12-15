#include "graphics_manager.h"

#include "engine/assets/asset.h"
#include "engine/core/base/random.h"
#include "engine/core/debugger/profiler.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_registry.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/frustum.h"
#include "engine/core/math/geometry.h"
#include "engine/core/math/matrix_transform.h"
#include "engine/drivers/empty/empty_graphics_manager.h"
#include "engine/drivers/opengl/opengl_graphics_manager.h"
#include "engine/renderer/draw_data.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/render_graph/pass_creator.h"
#include "engine/renderer/render_graph/render_graph_defines.h"
#include "engine/renderer/renderer.h"
#include "engine/scene/scene.h"
#include "shader_resource_defines.hlsl.h"

#if USING(PLATFORM_WINDOWS)
#include "engine/drivers/d3d11/d3d11_graphics_manager.h"
#include "engine/drivers/d3d12/d3d12_graphics_manager.h"
#include "engine/drivers/vk/vulkan_graphics_manager.h"
#elif USING(PLATFORM_APPLE)
#include "engine/drivers/metal/metal_graphics_manager.h"
#endif

// @TODO: refactor
#include "engine/renderer/path_tracer/path_tracer.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef GetMessage
#undef GetMessage
#endif

namespace my {

const char* ToString(RenderGraphName p_name) {
    ERR_FAIL_INDEX_V(p_name, RenderGraphName::COUNT, nullptr);
    static constexpr const char* s_table[] = {
#define RENDER_GRAPH_DECLARE(ENUM, STR) STR,
        RENDER_GRAPH_LIST
#undef RENDER_GRAPH_DECLARE
    };

    return s_table[std::to_underlying(p_name)];
}

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

// @TODO: refactor this
template<typename T>
static void CreateUniformBuffer(ConstantBuffer<T>& p_buffer) {
    GpuBufferDesc buffer_desc{};
    buffer_desc.slot = T::GetUniformBufferSlot();
    buffer_desc.elementCount = 1;
    buffer_desc.elementSize = sizeof(T);
    p_buffer.buffer = *GraphicsManager::GetSingleton().CreateConstantBuffer(buffer_desc);
}

auto GraphicsManager::InitializeImpl() -> Result<void> {
    m_enableValidationLayer = DVAR_GET_BOOL(gfx_gpu_validation);

    const int num_frames = (GetBackend() == Backend::D3D12) ? NUM_FRAMES_IN_FLIGHT : 1;
    m_frameContexts.resize(num_frames);
    for (int i = 0; i < num_frames; ++i) {
        m_frameContexts[i] = CreateFrameContext();
    }
    if (auto res = InitializeInternal(); !res) {
        return HBN_ERROR(res.error());
    }

    if (m_backend == Backend::METAL) {
        return Result<void>();
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

    // create meshes
    m_screenQuadBuffers = CreateMesh(MakePlaneMesh(Vector3f(1)));
    m_skyboxBuffers = CreateMesh(MakeSkyBoxMesh());

    m_brdfImage = m_app->GetAssetRegistry()->GetAssetByHandle<ImageAsset>(AssetHandle{ "@res://images/brdf.hdr" });

    std::vector<Point> points;
    points.resize(4096);
    m_lines = CreateLine(points);

    m_initialized = true;
    return Result<void>();
}

void GraphicsManager::EventReceived(std::shared_ptr<IEvent> p_event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(p_event.get()); e) {
        OnSceneChange(*e->GetScene());
    }
    if (ResizeEvent* e = dynamic_cast<ResizeEvent*>(p_event.get()); e) {
        OnWindowResize(e->GetWidth(), e->GetHeight());
    }
}

auto GraphicsManager::Create() -> Result<GraphicsManager*> {
    const std::string& backend = DVAR_GET_STRING(gfx_backend);

    auto select_renderer = [&]() -> GraphicsManager* {
        auto create_empty_renderer = []() -> GraphicsManager* {
            return new EmptyGraphicsManager("EmptyGraphicsManager", Backend::EMPTY, 1);
        };

        auto create_d3d11_renderer = []() -> GraphicsManager* {
#if USING(PLATFORM_WINDOWS)
            return new D3d11GraphicsManager();
#else
            return nullptr;
#endif
        };
        auto create_d3d12_renderer = []() -> GraphicsManager* {
#if USING(PLATFORM_WINDOWS)
            return new D3d12GraphicsManager();
#else
            return nullptr;
#endif
        };
        auto create_vulkan_renderer = []() -> GraphicsManager* {
#if USING(PLATFORM_WINDOWS)
            return new VulkanGraphicsManager();
#else
            return nullptr;
#endif
        };
        auto create_opengl_renderer = []() -> GraphicsManager* {
#if USING(PLATFORM_WINDOWS)
            return new OpenGlGraphicsManager();
#else
            return nullptr;
#endif
        };
        auto create_metal_renderer = []() -> GraphicsManager* {
#if USING(PLATFORM_APPLE)
            return new MetalGraphicsManager();
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

void GraphicsManager::RequestTexture(ImageAsset* p_image) {
    m_loadedImages.push(p_image);
}

void GraphicsManager::UpdateMesh(MeshBuffers* p_mesh, const std::vector<Vector3f>& p_positions, const std::vector<Vector3f>& p_normals) {
    unused(p_mesh);
    unused(p_positions);
    unused(p_normals);
    CRASH_NOW();
}

LineBuffers* GraphicsManager::CreateLine(const std::vector<Point>& p_points) {
    unused(p_points);
    CRASH_NOW();
    return nullptr;
}

void GraphicsManager::SetLine(const LineBuffers* p_buffer) {
    unused(p_buffer);
}

void GraphicsManager::UpdateLine(LineBuffers* p_buffer, const std::vector<Point>& p_points) {
    unused(p_buffer);
    unused(p_points);
    CRASH_NOW();
}

void GraphicsManager::DrawArrays(uint32_t p_count, uint32_t p_offset) {
    unused(p_count);
    unused(p_offset);
    CRASH_NOW();
}

// @TODO: refactor this
static void FillTextureAndSamplerDesc(const ImageAsset* p_image, GpuTextureDesc& p_texture_desc, SamplerDesc& p_sampler_desc) {
    DEV_ASSERT(p_image);
    bool is_hdr_file = false;

    switch (p_image->format) {
        case PixelFormat::R32_FLOAT:
        case PixelFormat::R32G32_FLOAT:
        case PixelFormat::R32G32B32_FLOAT:
        case PixelFormat::R32G32B32A32_FLOAT: {
            is_hdr_file = true;
        } break;
        default: {
        } break;
    }

    p_texture_desc.format = p_image->format;
    p_texture_desc.dimension = Dimension::TEXTURE_2D;
    p_texture_desc.width = p_image->width;
    p_texture_desc.height = p_image->height;
    p_texture_desc.arraySize = 1;
    p_texture_desc.bindFlags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    p_texture_desc.initialData = p_image->buffer.data();
    p_texture_desc.mipLevels = 1;

    if (is_hdr_file) {
        p_sampler_desc.minFilter = p_sampler_desc.magFilter = FilterMode::LINEAR;
        p_sampler_desc.addressU = p_sampler_desc.addressV = AddressMode::CLAMP;
        // p_texture_desc.bindFlags &= (~BIND_RENDER_TARGET);
        p_texture_desc.miscFlags |= RESOURCE_MISC_GENERATE_MIPS;
    } else {
        p_texture_desc.miscFlags |= RESOURCE_MISC_GENERATE_MIPS;
        p_sampler_desc.minFilter = FilterMode::MIPMAP_LINEAR;
        p_sampler_desc.magFilter = FilterMode::LINEAR;
    }
}

void GraphicsManager::Update(Scene& p_scene) {
    OPTICK_EVENT();

    // @TODO: make it a function
    auto loaded_images = m_loadedImages.pop_all();
    while (!loaded_images.empty()) {
        auto task = loaded_images.front();
        loaded_images.pop();
        ImageAsset* image = task;
        DEV_ASSERT(image);

        GpuTextureDesc texture_desc{};
        SamplerDesc sampler_desc{};
        FillTextureAndSamplerDesc(image, texture_desc, sampler_desc);

        image->gpu_texture = CreateTexture(texture_desc, sampler_desc);
    }

    {
        OPTICK_EVENT("Render");
        BeginFrame();

        auto data = renderer::GetRenderData();

        for (const auto& update_buffer : data->updateBuffer) {
            UpdateMesh((MeshBuffers*)update_buffer.id, update_buffer.positions, update_buffer.normals);
        }

        // @TODO: remove this
        UpdateEmitters(p_scene);

        if (data) {
            auto& frame = GetCurrentFrame();
            UpdateConstantBuffer(frame.batchCb.get(), data->batchCache.buffer);
            UpdateConstantBuffer(frame.materialCb.get(), data->materialCache.buffer);
            UpdateConstantBuffer(frame.boneCb.get(), data->boneCache.buffer);
            UpdateConstantBuffer(frame.passCb.get(), data->passCache);
            UpdateConstantBuffer(frame.emitterCb.get(), data->emitterCache);

            UpdateConstantBuffer<PointShadowConstantBuffer, 6 * MAX_POINT_LIGHT_SHADOW_COUNT>(
                frame.pointShadowCb.get(),
                data->pointShadowCache);
            UpdateConstantBuffer(frame.perFrameCb.get(),
                                 &data->perFrameCache,
                                 sizeof(PerFrameConstantBuffer));

            BindConstantBufferSlot<PerFrameConstantBuffer>(frame.perFrameCb.get(), 0);

            // @HACK
            switch (m_backend) {
                case Backend::VULKAN:
                case Backend::METAL:
                    break;
                default: {
                    auto graph = GetActiveRenderGraph();
                    if (DEV_VERIFY(graph)) {
                        graph->Execute(*data, *this);
                    }
                } break;
            }
        }

        Render();
        EndFrame();
        Present();
        MoveToNextFrame();
    }
}

void GraphicsManager::UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) {
    unused(p_desc);
    unused(p_buffer);
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
            UnbindTexture(texture->desc.dimension, texture->slot);
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
            BindTexture(texture->desc.dimension, texture->GetHandle(), texture->slot);
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
#define RENDER_GRAPH_DECLARE(ENUM, STR) \
    { STR, RenderGraphName::ENUM },
        RENDER_GRAPH_LIST
#undef RENDER_GRAPH_DECLARE
    };

    if (!method.empty()) {
        auto it = lookup.find(method);
        if (it == lookup.end()) {
            return HBN_ERROR(ErrorCode::ERR_INVALID_PARAMETER, "unknown render graph '{}'", method);
        } else {
            m_activeRenderGraphName = it->second;
        }
    }

    switch (GetBackend()) {
        case Backend::VULKAN:
        case Backend::EMPTY:
        case Backend::METAL:
            m_activeRenderGraphName = RenderGraphName::DUMMY;
            break;
        default:
            break;
    }

    // force to default
    if (m_backend != Backend::OPENGL && m_activeRenderGraphName == RenderGraphName::EXPERIMENTAL) {
        LOG_WARN("'experimental' not supported, fall back to 'default'");
        m_activeRenderGraphName = RenderGraphName::DEFAULT;
    }

    switch (m_activeRenderGraphName) {
        case RenderGraphName::DUMMY:
            m_renderGraphs[std::to_underlying(RenderGraphName::DUMMY)] = renderer::RenderPassCreator::CreateDummy();
            break;
        case RenderGraphName::EXPERIMENTAL:
            m_renderGraphs[std::to_underlying(RenderGraphName::EXPERIMENTAL)] = renderer::RenderPassCreator::CreateExperimental();
            break;
        case RenderGraphName::DEFAULT:
            m_renderGraphs[std::to_underlying(RenderGraphName::DEFAULT)] = renderer::RenderPassCreator::CreateDefault();
            break;
        default:
            DEV_ASSERT(0 && "Should not reach here");
            return HBN_ERROR(ErrorCode::ERR_INVALID_PARAMETER, "unknown render graph '{}'", method);
    }

    switch (m_backend) {
        case Backend::OPENGL:
        case Backend::D3D11:
            m_renderGraphs[std::to_underlying(RenderGraphName::PATHTRACER)] = renderer::RenderPassCreator::CreatePathTracer();
            break;
        default:
            break;
    }

    return Result<void>();
}

bool GraphicsManager::SetActiveRenderGraph(RenderGraphName p_name) {
    ERR_FAIL_INDEX_V(p_name, RenderGraphName::COUNT, false);
    const int index = std::to_underlying(p_name);
    if (!m_renderGraphs[index]) {
        return false;
    }

    if (p_name == m_activeRenderGraphName) {
        return false;
    }

    m_activeRenderGraphName = p_name;
    return true;
}

renderer::RenderGraph* GraphicsManager::GetActiveRenderGraph() {
    const int index = std::to_underlying(m_activeRenderGraphName);
    ERR_FAIL_INDEX_V(index, RenderGraphName::COUNT, nullptr);
    DEV_ASSERT(m_renderGraphs[index] != nullptr);
    return m_renderGraphs[index].get();
}

bool GraphicsManager::StartPathTracer(PathTracerMethod p_method) {
    unused(p_method);
    if (m_pathTracerGeometryBuffer) {
        return true;
    }

    // @TODO: refactor
    DEV_ASSERT(m_activeRenderGraphName == RenderGraphName::PATHTRACER);
    DEV_ASSERT(m_backend == Backend::OPENGL || m_backend == Backend::D3D11);

    SceneManager* scene_manager = m_app->GetSceneManager();
    Scene* scene = scene_manager->GetScenePtr();
    if (DEV_VERIFY(scene)) {
        GpuScene gpu_scene;
        ConstructScene(*scene, gpu_scene);

        const uint32_t geometry_count = (uint32_t)gpu_scene.geometries.size();
        const uint32_t bvh_count = (uint32_t)gpu_scene.bvhs.size();
        const uint32_t material_count = (uint32_t)gpu_scene.materials.size();

        {
            GpuBufferDesc desc{
                .slot = GetGlobalGeometriesSlot(),
                .elementSize = sizeof(gpu_geometry_t),
                .elementCount = geometry_count,
                .initialData = gpu_scene.geometries.data(),
            };
            m_pathTracerGeometryBuffer = *CreateStructuredBuffer(desc);
        }
        {
            GpuBufferDesc desc{
                .slot = GetGlobalGeometriesSlot(),
                .elementSize = sizeof(gpu_bvh_t),
                .elementCount = bvh_count,
                .initialData = gpu_scene.bvhs.data(),
            };
            m_pathTracerBvhBuffer = *CreateStructuredBuffer(desc);
        }
        {
            GpuBufferDesc desc{
                .slot = GetGlobalMaterialsSlot(),
                .elementSize = sizeof(gpu_material_t),
                .elementCount = material_count,
                .initialData = gpu_scene.materials.data(),
            };
            m_pathTracerMaterialBuffer = *CreateStructuredBuffer(desc);
        }

        LOG("Path tracer scene loaded, contains {} triangles, {} BVH", geometry_count, bvh_count);

        return true;
    }

    return false;
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
    switch (m_activeRenderGraphName) {
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
            texture = FindTexture(RESOURCE_PATH_TRACER).get();
            // texture = FindTexture(RESOURCE_TONE).get();
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
    }
}

void GraphicsManager::DrawQuad() {
    SetMesh(m_screenQuadBuffers);
    DrawElements(m_screenQuadBuffers->indexCount);
}

void GraphicsManager::DrawQuadInstanced(uint32_t p_instance_count) {
    SetMesh(m_screenQuadBuffers);
    DrawElementsInstanced(p_instance_count, m_screenQuadBuffers->indexCount, 0);
}

void GraphicsManager::DrawSkybox() {
    SetMesh(m_skyboxBuffers);
    DrawElements(m_skyboxBuffers->indexCount);
}

void GraphicsManager::OnSceneChange(const Scene& p_scene) {
    for (auto [entity, mesh] : p_scene.m_MeshComponents) {
        if (mesh.gpuResource != nullptr) {
            const NameComponent& name = *p_scene.GetComponent<NameComponent>(entity);
            LOG_WARN("[begin_scene] mesh '{}' () already has gpu resource", name.GetName());
            continue;
        }

        CreateMesh(mesh);
    }

    g_constantCache.update();
}

}  // namespace my
