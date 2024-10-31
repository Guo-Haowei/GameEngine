#pragma once
#include "core/base/concurrent_queue.h"
#include "core/base/singleton.h"
#include "core/framework/event_queue.h"
#include "core/framework/module.h"
#include "rendering/gpu_resource.h"
#include "rendering/pipeline_state.h"
#include "rendering/pipeline_state_manager.h"
#include "rendering/render_graph/draw_pass.h"
#include "rendering/render_graph/render_graph.h"
#include "rendering/sampler.h"
#include "rendering/uniform_buffer.h"
#include "scene/material_component.h"

// @TODO: refactor
#include "cbuffer.h"
#include "rendering/uniform_buffer.h"
#include "scene/scene.h"
struct MaterialConstantBuffer;

namespace my {

enum class Backend : uint8_t {
    EMPTY,
    OPENGL,
    D3D11,
};

enum ClearFlags : uint32_t {
    CLEAR_NONE = BIT(0),
    CLEAR_COLOR_BIT = BIT(1),
    CLEAR_DEPTH_BIT = BIT(2),
    CLEAR_STENCIL_BIT = BIT(3),
};

struct Viewport {
    Viewport(int p_width, int p_height) : width(p_width), height(p_height), topLeftX(0), topLeftY(0) {}

    int width;
    int height;
    int topLeftX;
    int topLeftY;
};

struct MeshBuffers {
    virtual ~MeshBuffers() = default;

    uint32_t indexCount = 0;
};

struct MeshBuffers;

// @TODO: refactor
extern ConstantBuffer<PerFrameConstantBuffer> g_per_frame_cache;
extern ConstantBuffer<PerSceneConstantBuffer> g_constantCache;
extern ConstantBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;
extern ConstantBuffer<PointShadowConstantBuffer> g_point_shadow_cache;
extern ConstantBuffer<EnvConstantBuffer> g_env_cache;
extern ConstantBuffer<ParticleConstantBuffer> g_particle_cache;

// @TODO: refactor
enum StencilFlags {
    STENCIL_FLAG_SELECTED = BIT(1),
};

// @TODO: refactor
struct DrawContext {
    uint32_t index_count;
    uint32_t index_offset;
    int material_idx;
};

// @TODO: refactor
struct BatchContext {
    int bone_idx;
    int batch_idx;
    const MeshBuffers* mesh_data;
    std::vector<DrawContext> subsets;
    uint32_t flags;
};

struct PassContext {
    int pass_idx{ 0 };
    LightComponent light_component;

    std::vector<BatchContext> draws;
};

#define SHADER_TEXTURE(TYPE, NAME, SLOT) \
    constexpr int NAME##_slot = SLOT;
#include "texture_binding.h"
#undef SHADER_TEXTURE

class GraphicsManager : public Singleton<GraphicsManager>, public Module, public EventListener {
public:
    using OnTextureLoadFunc = void (*)(Image* p_image);

    enum class RenderGraph : uint8_t {
        DEFAULT,
        VXGI,
    };

    GraphicsManager(std::string_view p_name, Backend p_backend) : Module(p_name), m_backend(p_backend) {}

    bool Initialize() final;

    void Update(Scene& p_scene);

    virtual void SetRenderTarget(const DrawPass* p_draw_pass, int p_index = 0, int p_mip_level = 0) = 0;
    virtual void UnsetRenderTarget() = 0;

    virtual void Clear(const DrawPass* p_draw_pass, uint32_t p_flags, float* p_clear_color = nullptr, int p_index = 0) = 0;
    virtual void SetViewport(const Viewport& p_viewport) = 0;

    virtual const MeshBuffers* CreateMesh(const MeshComponent& p_mesh) = 0;
    virtual void SetMesh(const MeshBuffers* p_mesh) = 0;
    virtual void DrawElements(uint32_t p_count, uint32_t p_offset = 0) = 0;
    virtual void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset = 0) = 0;

    virtual void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) = 0;
    virtual void SetUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) = 0;
    void SetPipelineState(PipelineStateName p_name);
    virtual void SetStencilRef(uint32_t p_ref) = 0;

    std::shared_ptr<RenderTarget> CreateRenderTarget(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler);
    std::shared_ptr<RenderTarget> FindRenderTarget(RenderTargetResourceName p_name) const;

    virtual std::shared_ptr<GpuBuffer> CreateBuffer(const GpuBufferDesc& p_desc) = 0;
    virtual std::shared_ptr<GpuTexture> CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) = 0;

    virtual std::shared_ptr<ConstantBufferBase> CreateConstantBuffer(int p_slot, size_t p_capacity) = 0;

    virtual void UpdateConstantBuffer(const ConstantBufferBase* p_buffer, const void* p_data, size_t p_size) = 0;
    template<typename T>
    void UpdateConstantBuffer(const ConstantBufferBase* p_buffer, const std::vector<T>& p_vector) {
        UpdateConstantBuffer(p_buffer, p_vector.data(), sizeof(T) * (uint32_t)p_vector.size());
    }
    virtual void BindConstantBufferRange(const ConstantBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) = 0;
    template<typename T>
    void BindConstantBufferSlot(const ConstantBufferBase* p_buffer, int slot) {
        BindConstantBufferRange(p_buffer, sizeof(T), slot * sizeof(T));
    }

    virtual std::shared_ptr<DrawPass> CreateDrawPass(const DrawPassDesc& p_desc) = 0;
    virtual void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) = 0;
    virtual void UnbindTexture(Dimension p_dimension, int p_slot) = 0;

    void RequestTexture(ImageHandle* p_handle, OnTextureLoadFunc p_func = nullptr);

    // @TODO: move to renderer
    uint64_t GetFinalImage() const;

    // @TODO: thread safety ?
    void EventReceived(std::shared_ptr<Event> p_event) final;

    // @TODO: move to renderer
    const rg::RenderGraph& GetActiveRenderGraph() { return m_renderGraph; }

    static std::shared_ptr<GraphicsManager> Create();

    Backend GetBackend() const { return m_backend; }

    // @TODO: move to renderer
    void SelectRenderGraph();

protected:
    virtual void OnSceneChange(const Scene& p_scene) = 0;
    virtual void OnWindowResize(int, int) {}
    virtual void SetPipelineStateImpl(PipelineStateName p_name) = 0;
    virtual void Render() = 0;
    virtual bool InitializeImpl() = 0;

    const Backend m_backend;
    RenderGraph m_method = RenderGraph::DEFAULT;

    // @TODO: cache
    PipelineStateName m_lastPipelineName = PIPELINE_STATE_MAX;

    rg::RenderGraph m_renderGraph;

    std::map<RenderTargetResourceName, std::shared_ptr<RenderTarget>> m_resourceLookup;

    struct ImageTask {
        ImageHandle* handle;
        OnTextureLoadFunc func;
    };
    ConcurrentQueue<ImageTask> m_loadedImages;

    std::shared_ptr<PipelineStateManager> m_pipelineStateManager;

public:
    // @TODO: refactor
    // tmp particle stuff
    int m_particle_count = 0;

    using FilterObjectFunc1 = std::function<bool(const ObjectComponent& p_object)>;
    using FilterObjectFunc2 = std::function<bool(const AABB& p_object_aabb)>;

    template<typename BUFFER>
    struct BufferCache {
        std::vector<BUFFER> buffer;
        std::unordered_map<ecs::Entity, uint32_t> lookup;

        uint32_t FindOrAdd(ecs::Entity p_entity, const BUFFER& p_buffer) {
            auto it = lookup.find(p_entity);
            if (it != lookup.end()) {
                return it->second;
            }

            uint32_t index = static_cast<uint32_t>(buffer.size());
            lookup[p_entity] = index;
            buffer.emplace_back(p_buffer);
            return index;
        }

        void Clear() {
            buffer.clear();
            lookup.clear();
        }
    };

    // @TODO: refactor names
    struct Context {
        std::shared_ptr<ConstantBufferBase> batch_uniform;
        BufferCache<PerBatchConstantBuffer> batch_cache;

        std::shared_ptr<ConstantBufferBase> material_uniform;
        BufferCache<MaterialConstantBuffer> material_cache;

        std::shared_ptr<ConstantBufferBase> bone_uniform;
        BufferCache<BoneConstantBuffer> bone_cache;

        std::shared_ptr<ConstantBufferBase> pass_uniform;
        std::vector<PerPassConstantBuffer> pass_cache;
    } m_context;

    Context& GetContext() { return m_context; }

    // @TODO: save pass item somewhere and use index instead of keeping many copies
    std::array<std::unique_ptr<PassContext>, MAX_LIGHT_CAST_SHADOW_COUNT> m_pointShadowPasses;
    std::array<PassContext, 1> m_shadowPasses;  // @TODO: support multi ortho light

    PassContext m_voxelPass;
    PassContext m_mainPass;

private:
    void Cleanup();
    void UpdateConstants(const Scene& p_scene);
    void UpdateParticles(const Scene& p_scene);
    void UpdateLights(const Scene& p_scene);
    void UpdateVoxelPass(const Scene& p_scene);
    void UpdateMainPass(const Scene& p_scene);

    void FillPass(const Scene& p_scene, PassContext& p_pass, FilterObjectFunc1 p_filter1, FilterObjectFunc2 p_filter2);
};

}  // namespace my
