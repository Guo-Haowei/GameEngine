#pragma once
#include "assets/image.h"
#include "core/base/concurrent_queue.h"
#include "core/base/singleton.h"
#include "core/framework/event_queue.h"
#include "core/framework/module.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_graph/render_graph.h"
#include "rendering/sampler.h"
#include "scene/material_component.h"
#include "scene/scene_components.h"

// @TODO: refactor
struct MaterialConstantBuffer;

namespace my {

struct RenderData;

// @TODO: move generic stuff to renderer
class GraphicsManager : public Singleton<GraphicsManager>, public Module, public EventListener {
public:
    using OnTextureLoadFunc = void (*)(Image* p_image);

    enum class Backend : uint8_t {
        EMPTY,
        OPENGL,
        D3D11,
    };

    enum class RenderGraph : uint8_t {
        DUMMY,
        BASE_COLOR,
        VXGI,
    };

    GraphicsManager(std::string_view p_name, Backend p_backend) : Module(p_name), m_backend(p_backend) {}

    void update(float p_delta);

    void set_pipeline_state(PipelineStateName p_name);

    // @TODO: better name
    virtual std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler) = 0;
    std::shared_ptr<RenderTarget> find_resource(const std::string& p_name) const;

    virtual std::shared_ptr<Subpass> create_subpass(const SubpassDesc& p_desc) = 0;

    // @TODO: sampler
    virtual void create_texture(ImageHandle* p_handle) = 0;
    void request_texture(ImageHandle* p_handle, OnTextureLoadFunc p_func = nullptr);

    // virtual std::shared_ptr<rg::Subpass> create_subpass(const rg::SubpassDesc& p_desc) = 0;

    virtual uint64_t get_final_image() const = 0;

    // @TODO: thread safety ?
    void event_received(std::shared_ptr<Event> p_event) final;

    // @TODO: refactor this
    virtual void fill_material_constant_buffer(const MaterialComponent* p_material, MaterialConstantBuffer& p_cb) = 0;

    std::shared_ptr<RenderData> get_render_data() { return m_render_data; }
    const rg::RenderGraph& get_active_render_graph() { return m_render_graph; }

    static std::shared_ptr<GraphicsManager> create();

protected:
    virtual void on_scene_change(const Scene& p_scene) = 0;
    virtual void on_window_resize(int, int) {}
    virtual void set_pipeline_state_impl(PipelineStateName p_name) = 0;
    virtual void render() = 0;
    // @TODO: move to renderer
    void select_render_graph();

    const Backend m_backend;
    RenderGraph m_method;

    std::shared_ptr<RenderData> m_render_data;

    rg::RenderGraph m_render_graph;

    std::map<std::string, std::shared_ptr<RenderTarget>> m_resource_lookup;

    PipelineStateName m_last_pipeline_name = PIPELINE_STATE_MAX;

    struct ImageTask {
        ImageHandle* handle;
        OnTextureLoadFunc func;
    };
    ConcurrentQueue<ImageTask> m_loaded_images;
};

}  // namespace my
