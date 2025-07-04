#include "base_graphics_manager.h"

#include "engine/assets/asset.h"
#include "engine/core/base/random.h"
#include "engine/core/debugger/profiler.h"
#include "engine/runtime/application.h"
#include "engine/runtime/asset_registry.h"
#include "engine/math/frustum.h"
#include "engine/math/geometry.h"
#include "engine/math/matrix_transform.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/render_data.h"
#include "engine/renderer/render_graph/render_graph_builder.h"
#include "engine/renderer/render_graph/render_graph_defines.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/renderer_misc.h"
#include "engine/renderer/sampler.h"
#include "engine/scene/scene.h"

namespace my {
#include "shader_resource_defines.hlsl.h"
}  // namespace my

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
static auto CreateUniformCheckSize(BaseGraphicsManager& p_graphics_manager, uint32_t p_max_count) {
    static_assert(sizeof(T) % 256 == 0);
    GpuBufferDesc buffer_desc{};
    buffer_desc.slot = T::GetUniformBufferSlot();
    buffer_desc.elementCount = p_max_count;
    buffer_desc.elementSize = sizeof(T);
    return p_graphics_manager.CreateConstantBuffer(buffer_desc);
}

ConstantBuffer<PerSceneConstantBuffer> g_constantCache;

// @TODO: refactor this
template<typename T>
static void CreateUniformBuffer(ConstantBuffer<T>& p_buffer) {
    GpuBufferDesc buffer_desc{};
    buffer_desc.slot = T::GetUniformBufferSlot();
    buffer_desc.elementCount = 1;
    buffer_desc.elementSize = sizeof(T);
    p_buffer.buffer = *IGraphicsManager::GetSingleton().CreateConstantBuffer(buffer_desc);
}

auto BaseGraphicsManager::InitializeImpl() -> Result<void> {
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
    m_screenQuadBuffers = *CreateMesh(MakePlaneMesh(Vector3f(1)));
    m_skyboxBuffers = *CreateMesh(MakeSkyBoxMesh());
    m_boxBuffers = *CreateMesh(MakeBoxMesh());

    m_brdfImage = m_app->GetAssetRegistry()->GetAssetByHandle<ImageAsset>(AssetHandle{ "@res://images/brdf.hdr" });

    // @TODO: refactor
    {
        constexpr int max_count = 4096 * 128;
        MeshComponent mesh;
        mesh.flags |= MeshComponent::DYNAMIC;
        mesh.positions.resize(max_count);
        mesh.color_0.resize(max_count);
        mesh.CreateRenderData();

        auto res = CreateMesh(mesh);
        if (!res) {
            return HBN_ERROR(res.error());
        }
        m_debugBuffers = *res;
    }

    m_initialized = true;
    return Result<void>();
}

void BaseGraphicsManager::EventReceived(std::shared_ptr<IEvent> p_event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(p_event.get()); e) {
        OnSceneChange(*e->GetScene());
    }
    if (ResizeEvent* e = dynamic_cast<ResizeEvent*>(p_event.get()); e) {
        OnWindowResize(e->GetWidth(), e->GetHeight());
    }
}


void BaseGraphicsManager::SetPipelineState(PipelineStateName p_name) {
    SetPipelineStateImpl(p_name);
}

void BaseGraphicsManager::RequestTexture(ImageAsset* p_image) {
    m_loadedImages.push(p_image);
}

void BaseGraphicsManager::UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) {
    unused(p_desc);
    unused(p_buffer);
    CRASH_NOW();
}

auto BaseGraphicsManager::CreateMesh(const MeshComponent& p_mesh) -> Result<std::shared_ptr<GpuMesh>> {
    constexpr uint32_t count = std::to_underlying(VertexAttributeName::COUNT);
    std::array<VertexAttributeName, count> attribs = {
        VertexAttributeName::POSITION,
        VertexAttributeName::NORMAL,
        VertexAttributeName::TEXCOORD_0,
        VertexAttributeName::TANGENT,
        VertexAttributeName::JOINTS_0,
        VertexAttributeName::WEIGHTS_0,
        VertexAttributeName::COLOR_0,
        VertexAttributeName::TEXCOORD_1,
    };

    std::array<const void*, count> data = {
        p_mesh.positions.data(),
        p_mesh.normals.data(),
        p_mesh.texcoords_0.data(),
        p_mesh.tangents.data(),
        p_mesh.joints_0.data(),
        p_mesh.weights_0.data(),
        p_mesh.color_0.data(),
        p_mesh.texcoords_1.data(),
    };

    std::array<GpuBufferDesc, count> vb_descs;

    const bool is_dynamic = p_mesh.flags & MeshComponent::DYNAMIC;

    GpuMeshDesc desc;
    desc.enabledVertexCount = count;
    desc.drawCount = static_cast<uint32_t>(p_mesh.indices.empty() ? p_mesh.positions.size() : p_mesh.indices.size());

    for (int index = 0; index < (int)attribs.size(); ++index) {
        const auto& in = p_mesh.attributes[std::to_underlying(attribs[index])];
        auto& layout = desc.vertexLayout[index];
        layout.slot = index;
        layout.offsetInByte = in.offsetInByte;
        layout.strideInByte = in.strideInByte;

        auto& buffer_desc = vb_descs[index];
        buffer_desc.slot = index;
        buffer_desc.type = GpuBufferType::VERTEX;
        buffer_desc.elementCount = in.elementCount;
        buffer_desc.elementSize = in.strideInByte;
        buffer_desc.initialData = data[index];
        buffer_desc.dynamic = is_dynamic;
    }

    GpuBufferDesc ib_desc;
    GpuBufferDesc* ib_desc_ptr = nullptr;
    if (!p_mesh.indices.empty()) {
        ib_desc = GpuBufferDesc{
            .type = GpuBufferType::INDEX,
            .elementSize = sizeof(uint32_t),
            .elementCount = (uint32_t)p_mesh.indices.size(),
            .initialData = p_mesh.indices.data(),
        };
        ib_desc_ptr = &ib_desc;
    }

    auto ret = CreateMeshImpl(desc, count, vb_descs.data(), ib_desc_ptr);
    if (!ret) {
        return HBN_ERROR(ret.error());
    }

    p_mesh.gpuResource = *ret;
    return ret;
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
    p_texture_desc.miscFlags |= RESOURCE_MISC_GENERATE_MIPS;

    if (is_hdr_file) {
        p_sampler_desc.minFilter = MinFilter::LINEAR;
        p_sampler_desc.magFilter = MagFilter::LINEAR;
        p_sampler_desc.addressU = p_sampler_desc.addressV = AddressMode::CLAMP;
        // p_texture_desc.bindFlags &= (~BIND_RENDER_TARGET);
    } else {
        p_sampler_desc.minFilter = MinFilter::LINEAR_MIPMAP_LINEAR;
        p_sampler_desc.magFilter = MagFilter::LINEAR;
    }
}

void BaseGraphicsManager::Update(Scene& p_scene) {
    HBN_PROFILE_EVENT();

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
        HBN_PROFILE_EVENT("Render");
        BeginFrame();

        auto data = renderer::GetRenderData();

        for (const auto& update_buffer : data->updateBuffer) {
            GpuMesh* mesh = (GpuMesh*)update_buffer.id;
            if (mesh) {
                UpdateBuffer(renderer::CreateDesc(update_buffer.positions), mesh->vertexBuffers[0].get());
                UpdateBuffer(renderer::CreateDesc(update_buffer.normals), mesh->vertexBuffers[1].get());
            }
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

void BaseGraphicsManager::UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) {
    unused(p_desc);
    unused(p_buffer);
}

void BaseGraphicsManager::BeginFrame() {
}

void BaseGraphicsManager::EndFrame() {
}

void BaseGraphicsManager::MoveToNextFrame() {
}

std::shared_ptr<FrameContext> BaseGraphicsManager::CreateFrameContext() {
    return std::make_unique<FrameContext>();
}

void BaseGraphicsManager::BeginDrawPass(const Framebuffer* p_framebuffer) {
    for (auto& texture : p_framebuffer->outSrvs) {
        if (texture->slot >= 0) {
            UnbindTexture(texture->desc.dimension, texture->slot);
            // RT_DEBUG("  -- unbound resource '{}'({})", RenderTargetResourceNameToString(it->desc.name), it->slot);
        }
    }

    for (auto& transition : p_framebuffer->desc.transitions) {
        if (transition.beginPassFunc) {
            transition.beginPassFunc(this, transition.resource.get(), transition.slot);
        }
    }
}

void BaseGraphicsManager::EndDrawPass(const Framebuffer* p_framebuffer) {
    UnsetRenderTarget();
    for (auto& texture : p_framebuffer->outSrvs) {
        if (texture->slot >= 0) {
            BindTexture(texture->desc.dimension, texture->GetHandle(), texture->slot);
            // RT_DEBUG("  -- bound resource '{}'({})", RenderTargetResourceNameToString(it->desc.name), it->slot);
        }
    }

    for (auto& transition : p_framebuffer->desc.transitions) {
        if (transition.endPassFunc) {
            transition.endPassFunc(this, transition.resource.get(), transition.slot);
        }
    }
}

auto BaseGraphicsManager::SelectRenderGraph() -> Result<void> {
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

#if USING(PLATFORM_WASM)
    m_activeRenderGraphName = RenderGraphName::EMPTY;
#endif

    renderer::RenderGraphBuilder::CreateResources();
    const Vector2i frame_size = DVAR_GET_IVEC2(resolution);
    renderer::RenderGraphBuilderConfig config;
    config.frameWidth = frame_size.x;
    config.frameHeight = frame_size.y;
    config.is_runtime = m_app->IsRuntime();

    switch (m_activeRenderGraphName) {
        case RenderGraphName::DUMMY:
            m_renderGraphs[std::to_underlying(RenderGraphName::DUMMY)] = renderer::RenderGraphBuilder::CreateDummy(config);
            break;
        case RenderGraphName::DEFAULT:
            m_renderGraphs[std::to_underlying(RenderGraphName::DEFAULT)] = renderer::RenderGraphBuilder::CreateDefault(config);
            break;
        case RenderGraphName::EMPTY:
            m_renderGraphs[std::to_underlying(RenderGraphName::EMPTY)] = renderer::RenderGraphBuilder::CreateEmpty(config);
            break;
        default:
            DEV_ASSERT(0 && "Should not reach here");
            return HBN_ERROR(ErrorCode::ERR_INVALID_PARAMETER, "unknown render graph '{}'", method);
    }

    switch (m_backend) {
        case Backend::OPENGL:
        case Backend::D3D11:
#if !USING(PLATFORM_WASM)
            m_renderGraphs[std::to_underlying(RenderGraphName::PATHTRACER)] = renderer::RenderGraphBuilder::CreatePathTracer(config);
#endif
            break;
        default:
            break;
    }

    return Result<void>();
}

bool BaseGraphicsManager::SetActiveRenderGraph(RenderGraphName p_name) {
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

renderer::RenderGraph* BaseGraphicsManager::GetActiveRenderGraph() {
    const int index = std::to_underlying(m_activeRenderGraphName);
    ERR_FAIL_INDEX_V(index, RenderGraphName::COUNT, nullptr);
    DEV_ASSERT(m_renderGraphs[index] != nullptr);
    return m_renderGraphs[index].get();
}

std::shared_ptr<GpuTexture> BaseGraphicsManager::CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
    auto texture = CreateTextureImpl(p_texture_desc, p_sampler_desc);
    if (p_texture_desc.type != AttachmentType::NONE) {
        DEV_ASSERT(m_resourceLookup.find(p_texture_desc.name) == m_resourceLookup.end());
        m_resourceLookup[p_texture_desc.name] = texture;
    }
    return texture;
}

std::shared_ptr<GpuTexture> BaseGraphicsManager::FindTexture(RenderTargetResourceName p_name) const {
    if (m_resourceLookup.empty()) {
        return nullptr;
    }

    auto it = m_resourceLookup.find(p_name);
    if (it == m_resourceLookup.end()) {
        return nullptr;
    }
    return it->second;
}

uint64_t BaseGraphicsManager::GetFinalImage() const {
    const GpuTexture* texture = nullptr;
    switch (m_activeRenderGraphName) {
        case RenderGraphName::DUMMY: {
            texture = FindTexture(RESOURCE_GBUFFER_NORMAL).get();
        } break;
        case RenderGraphName::DEFAULT: {
#if 0
            // @TODO: debug panel
            texture = FindTexture(RESOURCE_SSAO).get();
#else
            texture = FindTexture(RESOURCE_FINAL).get();
#endif
        } break;
        case RenderGraphName::PATHTRACER: {
            texture = FindTexture(RESOURCE_PATH_TRACER).get();
        } break;
        case RenderGraphName::EMPTY: {
            texture = FindTexture(RESOURCE_FINAL).get();
        } break;
        default: {
            CRASH_NOW();
        } break;
    }

    if (texture) {
        return texture->GetHandle();
    }

    return 0;
}

void BaseGraphicsManager::UpdateEmitters(const Scene& p_scene) {
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

void BaseGraphicsManager::DrawQuad() {
    SetMesh(m_screenQuadBuffers.get());
    DrawElements(m_screenQuadBuffers->desc.drawCount);
}

void BaseGraphicsManager::DrawQuadInstanced(uint32_t p_instance_count) {
    SetMesh(m_screenQuadBuffers.get());
    DrawElementsInstanced(p_instance_count, m_screenQuadBuffers->desc.drawCount, 0);
}

void BaseGraphicsManager::DrawSkybox() {
    SetMesh(m_skyboxBuffers.get());
    DrawElements(m_skyboxBuffers->desc.drawCount);
}

void BaseGraphicsManager::OnSceneChange(const Scene& p_scene) {
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
