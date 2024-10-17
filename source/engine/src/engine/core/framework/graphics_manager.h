#pragma once
#include "core/base/concurrent_queue.h"
#include "core/base/singleton.h"
#include "core/framework/event_queue.h"
#include "core/framework/module.h"
#include "rendering/pipeline_state.h"
#include "rendering/pipeline_state_manager.h"
#include "rendering/render_graph/render_graph.h"
#include "rendering/render_graph/subpass.h"
#include "rendering/sampler.h"
#include "rendering/texture.h"
#include "rendering/uniform_buffer.h"
#include "scene/material_component.h"

// @TODO: refactor
#include "rendering/render_data.h"
struct MaterialConstantBuffer;

namespace my {

// @TODO: refactor
struct RenderData;

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

    virtual void setRenderTarget(const Subpass* p_subpass, int p_index = 0, int p_mip_level = 0) = 0;
    virtual void clear(const Subpass* p_subpass, uint32_t p_flags, float* p_clear_color = nullptr) = 0;
    virtual void setViewport(const Viewport& p_viewport) = 0;

    virtual const MeshBuffers* createMesh(const MeshComponent& p_mesh) = 0;
    virtual void setMesh(const MeshBuffers* p_mesh) = 0;
    virtual void drawElements(uint32_t p_count, uint32_t p_offset = 0) = 0;

    void setPipelineState(PipelineStateName p_name);
    virtual void setStencilRef(uint32_t p_ref) = 0;

    std::shared_ptr<RenderTarget> createRenderTarget(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler);
    std::shared_ptr<RenderTarget> findRenderTarget(const std::string& p_name) const;

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

    virtual std::shared_ptr<Subpass> createSubpass(const SubpassDesc& p_desc) = 0;
    virtual std::shared_ptr<Texture> createTexture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) = 0;
    virtual void bindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) = 0;

    void requestTexture(ImageHandle* p_handle, OnTextureLoadFunc p_func = nullptr);

    // @TODO: move to renderer
    uint64_t getFinalImage() const;

    // @TODO: thread safety ?
    void eventReceived(std::shared_ptr<Event> p_event) final;

    // @TODO: move to renderer
    std::shared_ptr<RenderData> getRenderData() { return m_render_data; }
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

    // @TODO: move the following to renderer
    std::shared_ptr<RenderData> m_render_data;
    rg::RenderGraph m_render_graph;

    std::map<std::string, std::shared_ptr<RenderTarget>> m_resource_lookup;

    struct ImageTask {
        ImageHandle* handle;
        OnTextureLoadFunc func;
    };
    ConcurrentQueue<ImageTask> m_loaded_images;

    std::shared_ptr<PipelineStateManager> m_pipeline_state_manager;
};

}  // namespace my
