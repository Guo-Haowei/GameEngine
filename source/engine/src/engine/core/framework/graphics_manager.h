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
#include "scene/scene_components.h"

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
        DUMMY,
        VXGI,
    };

    GraphicsManager(std::string_view p_name, Backend p_backend) : Module(p_name), m_backend(p_backend) {}

    bool initialize() final;

    void update(float p_delta);

    virtual void set_render_target(const Subpass* p_subpass, int p_index = 0, int p_mip_level = 0) = 0;
    virtual void clear(const Subpass* p_subpass, uint32_t p_flags, float* p_clear_color = nullptr) = 0;
    virtual void set_viewport(const Viewport& p_viewport) = 0;

    virtual const MeshBuffers* create_mesh(const MeshComponent& p_mesh) = 0;
    virtual void set_mesh(const MeshBuffers* p_mesh) = 0;
    virtual void draw_elements(uint32_t p_count, uint32_t p_offset = 0) = 0;

    void set_pipeline_state(PipelineStateName p_name);
    virtual void set_stencil_ref(uint32_t p_ref) = 0;

    std::shared_ptr<RenderTarget> create_render_target(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler);
    std::shared_ptr<RenderTarget> find_render_target(const std::string& p_name) const;

    virtual std::shared_ptr<UniformBufferBase> uniform_create(int p_slot, size_t p_capacity) = 0;
    virtual void uniform_update(const UniformBufferBase* p_buffer, const void* p_data, size_t p_size) = 0;
    template<typename T>
    void uniform_update(const UniformBufferBase* p_buffer, const std::vector<T>& p_vector) {
        uniform_update(p_buffer, p_vector.data(), sizeof(T) * (uint32_t)p_vector.size());
    }
    virtual void uniform_bind_range(const UniformBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) = 0;
    template<typename T>
    void uniform_bind_slot(const UniformBufferBase* p_buffer, int slot) {
        uniform_bind_range(p_buffer, sizeof(T), slot * sizeof(T));
    }

    virtual std::shared_ptr<Subpass> create_subpass(const SubpassDesc& p_desc) = 0;
    virtual std::shared_ptr<Texture> create_texture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) = 0;

    void request_texture(ImageHandle* p_handle, OnTextureLoadFunc p_func = nullptr);

    // @TODO: move to renderer
    uint64_t get_final_image() const;

    // @TODO: thread safety ?
    void event_received(std::shared_ptr<Event> p_event) final;

    // @TODO: move to renderer
    std::shared_ptr<RenderData> get_render_data() { return m_render_data; }
    const rg::RenderGraph& get_active_render_graph() { return m_render_graph; }

    static std::shared_ptr<GraphicsManager> create();

    Backend get_backend() const { return m_backend; }

    // @TODO: move to renderer
    void select_render_graph();

protected:
    virtual void on_scene_change(const Scene& p_scene) = 0;
    virtual void on_window_resize(int, int) {}
    virtual void set_pipeline_state_impl(PipelineStateName p_name) = 0;
    virtual void render() = 0;
    virtual bool initialize_internal() = 0;

    const Backend m_backend;
    RenderGraph m_method = RenderGraph::DUMMY;

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
