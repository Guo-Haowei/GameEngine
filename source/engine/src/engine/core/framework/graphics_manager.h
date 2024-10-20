#pragma once
#include "core/base/concurrent_queue.h"
#include "core/base/singleton.h"
#include "core/framework/event_queue.h"
#include "core/framework/module.h"
#include "rendering/pipeline_state.h"
#include "rendering/pipeline_state_manager.h"
#include "rendering/render_graph/draw_pass.h"
#include "rendering/render_graph/render_graph.h"
#include "rendering/sampler.h"
#include "rendering/texture.h"
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
    Viewport(int p_width, int p_height) : width(p_width), height(p_height), top_left_x(0), top_left_y(0) {}

    int width;
    int height;
    int top_left_x;
    int top_left_y;
};

struct MeshBuffers {
    virtual ~MeshBuffers() = default;

    uint32_t index_count = 0;
};

struct MeshBuffers;

extern UniformBuffer<PerFrameConstantBuffer> g_per_frame_cache;
extern UniformBuffer<PerSceneConstantBuffer> g_constantCache;
extern UniformBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;
extern UniformBuffer<BloomConstantBuffer> g_bloom_cache;
extern UniformBuffer<PointShadowConstantBuffer> g_point_shadow_cache;
extern UniformBuffer<EnvConstantBuffer> g_env_cache;

enum StencilFlags {
    STENCIL_FLAG_SELECTED = BIT(1),
};

struct DrawContext {
    uint32_t index_count;
    uint32_t index_offset;
    int material_idx;
};

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

// @TODO: move generic stuff to renderer
class GraphicsManager : public Singleton<GraphicsManager>, public Module, public EventListener {
public:
    using OnTextureLoadFunc = void (*)(Image* p_image);

    enum class RenderGraph : uint8_t {
        DEFAULT,
        VXGI,
    };

    GraphicsManager(std::string_view p_name, Backend p_backend) : Module(p_name), m_backend(p_backend) {}

    bool initialize() final;

    void update(Scene& p_scene);

    virtual void setRenderTarget(const DrawPass* p_draw_pass, int p_index = 0, int p_mip_level = 0) = 0;
    virtual void unsetRenderTarget() = 0;

    virtual void clear(const DrawPass* p_draw_pass, uint32_t p_flags, float* p_clear_color = nullptr) = 0;
    virtual void setViewport(const Viewport& p_viewport) = 0;

    virtual const MeshBuffers* createMesh(const MeshComponent& p_mesh) = 0;
    virtual void setMesh(const MeshBuffers* p_mesh) = 0;
    virtual void drawElements(uint32_t p_count, uint32_t p_offset = 0) = 0;

    void setPipelineState(PipelineStateName p_name);
    virtual void setStencilRef(uint32_t p_ref) = 0;

    std::shared_ptr<RenderTarget> createRenderTarget(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler);
    std::shared_ptr<RenderTarget> findRenderTarget(RenderTargetResourceName p_name) const;

    virtual std::shared_ptr<UniformBufferBase> createUniform(int p_slot, size_t p_capacity) = 0;
    virtual void updateUniform(const UniformBufferBase* p_buffer, const void* p_data, size_t p_size) = 0;
    template<typename T>
    void updateUniform(const UniformBufferBase* p_buffer, const std::vector<T>& p_vector) {
        updateUniform(p_buffer, p_vector.data(), sizeof(T) * (uint32_t)p_vector.size());
    }
    virtual void bindUniformRange(const UniformBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) = 0;
    template<typename T>
    void bindUniformSlot(const UniformBufferBase* p_buffer, int slot) {
        bindUniformRange(p_buffer, sizeof(T), slot * sizeof(T));
    }

    virtual std::shared_ptr<DrawPass> createDrawPass(const DrawPassDesc& p_desc) = 0;
    virtual std::shared_ptr<Texture> createTexture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) = 0;
    virtual void bindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) = 0;
    virtual void unbindTexture(Dimension p_dimension, int p_slot) = 0;

    void requestTexture(ImageHandle* p_handle, OnTextureLoadFunc p_func = nullptr);

    // @TODO: move to renderer
    uint64_t getFinalImage() const;

    // @TODO: thread safety ?
    void eventReceived(std::shared_ptr<Event> p_event) final;

    // @TODO: move to renderer
    const rg::RenderGraph& getActiveRenderGraph() { return m_render_graph; }

    static std::shared_ptr<GraphicsManager> create();

    Backend getBackend() const { return m_backend; }

    // @TODO: move to renderer
    void selectRenderGraph();

protected:
    virtual void onSceneChange(const Scene& p_scene) = 0;
    virtual void onWindowResize(int, int) {}
    virtual void setPipelineStateImpl(PipelineStateName p_name) = 0;
    virtual void render() = 0;
    virtual bool initializeImpl() = 0;

    const Backend m_backend;
    RenderGraph m_method = RenderGraph::DEFAULT;

    // @TODO: cache
    PipelineStateName m_last_pipeline_name = PIPELINE_STATE_MAX;

    rg::RenderGraph m_render_graph;

    std::map<RenderTargetResourceName, std::shared_ptr<RenderTarget>> m_resource_lookup;

    struct ImageTask {
        ImageHandle* handle;
        OnTextureLoadFunc func;
    };
    ConcurrentQueue<ImageTask> m_loaded_images;

    std::shared_ptr<PipelineStateManager> m_pipeline_state_manager;

    // @TODO: refactor
public:
    using FilterObjectFunc1 = std::function<bool(const ObjectComponent& object)>;
    using FilterObjectFunc2 = std::function<bool(const AABB& object_aabb)>;

    template<typename BUFFER>
    struct BufferCache {
        std::vector<BUFFER> buffer;
        std::unordered_map<ecs::Entity, uint32_t> lookup;

        uint32_t findOrAdd(ecs::Entity p_entity, const BUFFER& p_buffer) {
            auto it = lookup.find(p_entity);
            if (it != lookup.end()) {
                return it->second;
            }

            uint32_t index = static_cast<uint32_t>(buffer.size());
            lookup[p_entity] = index;
            buffer.emplace_back(p_buffer);
            return index;
        }

        void clear() {
            buffer.clear();
            lookup.clear();
        }
    };

    // @TODO: refactor names
    struct Context {
        std::shared_ptr<UniformBufferBase> batch_uniform;
        BufferCache<PerBatchConstantBuffer> batch_cache;

        std::shared_ptr<UniformBufferBase> material_uniform;
        BufferCache<MaterialConstantBuffer> material_cache;

        std::shared_ptr<UniformBufferBase> bone_uniform;
        BufferCache<BoneConstantBuffer> bone_cache;

        std::shared_ptr<UniformBufferBase> pass_uniform;
        std::vector<PerPassConstantBuffer> pass_cache;
    } m_context;

    Context& getContext() { return m_context; }

    // @TODO: save pass item somewhere and use index instead of keeping many copies
    std::array<std::unique_ptr<PassContext>, MAX_LIGHT_CAST_SHADOW_COUNT> point_shadow_passes;
    std::array<PassContext, 1> shadow_passes;  // @TODO: support multi omni lights

    PassContext voxel_pass;
    PassContext main_pass;

private:
    void cleanup();
    void updateConstants(const Scene& p_scene);
    void updateLights(const Scene& p_scene);
    void updateVoxelPass(const Scene& p_scene);
    void updateMainPass(const Scene& p_scene);

    void fillPass(const Scene& p_scene, PassContext& p_pass, FilterObjectFunc1 p_filter1, FilterObjectFunc2 p_filter2);
};

}  // namespace my
