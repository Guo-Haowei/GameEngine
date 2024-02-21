#pragma once
#include "assets/image.h"
#include "core/base/rid_owner.h"
#include "core/base/singleton.h"
#include "core/framework/event_queue.h"
#include "core/framework/module.h"
#include "rendering/GpuTexture.h"
#include "rendering/gl_utils.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_graph/render_graph.h"
#include "scene/material_component.h"
#include "scene/scene_components.h"

namespace my {

struct RenderData;

class GraphicsManager : public Singleton<GraphicsManager>, public Module, public EventListener {
public:
    enum RenderGraph {
        RENDER_GRAPH_NONE,
        RENDER_GRAPH_BASE_COLOR,
        RENDER_GRAPH_VXGI,
    };

    GraphicsManager(std::string_view p_name) : Module(p_name) {}

    virtual void render() = 0;

    // @TODO: filter
    virtual void create_texture(ImageHandle* handle) = 0;

    void event_received(std::shared_ptr<Event> event) override;

    virtual std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc& desc) = 0;

    virtual uint32_t get_final_image() const = 0;

    std::shared_ptr<RenderData> get_render_data() { return m_render_data; }

    const rg::RenderGraph& get_active_render_graph() { return m_render_graph; }

    virtual std::shared_ptr<RenderTarget> find_resource(const std::string& name) const = 0;

    // @TODO: refactor this
    virtual void fill_material_constant_buffer(const MaterialComponent* material, MaterialConstantBuffer& cb) = 0;

protected:
    RenderGraph m_method = RENDER_GRAPH_NONE;

    std::shared_ptr<RenderData> m_render_data;

    rg::RenderGraph m_render_graph;

    std::map<std::string, std::shared_ptr<RenderTarget>> m_resource_lookup;
};

}  // namespace my
