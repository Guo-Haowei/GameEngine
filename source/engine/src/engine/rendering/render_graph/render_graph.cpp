#include "render_graph.h"

#include "rendering/GLPrerequisites.h"

namespace my::rg {

static GLuint create_resource_impl(const ResourceDesc& desc) {
    constexpr float shadow_boarder[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLuint texture_id = 0;
    GLenum type = GL_TEXTURE_2D;
    glGenTextures(1, &texture_id);
    switch (desc.type) {
        case RT_COLOR_ATTACHMENT: {
            glBindTexture(type, texture_id);
            glTexImage2D(type,                                       // target
                         0,                                          // level
                         format_to_gl_internal_format(desc.format),  // internal format
                         desc.width, desc.height,                    // dimension
                         0,                                          // boarder
                         format_to_gl_format(desc.format),           // format
                         format_to_gl_data_type(desc.format),        // type
                         nullptr                                     // pixels
            );
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(type, 0);
        } break;
        case RT_DEPTH_ATTACHMENT: {
            glBindTexture(type, texture_id);
            glTexImage2D(type,                                       // target
                         0,                                          // level
                         format_to_gl_internal_format(desc.format),  // internal format
                         desc.width, desc.height,                    // dimension
                         0,                                          // boarder
                         format_to_gl_format(desc.format),           // format
                         format_to_gl_data_type(desc.format),        // type
                         nullptr                                     // pixels
            );

            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(type, 0);
        } break;
        case RT_SHADOW_MAP: {
            glBindTexture(type, texture_id);
            glTexImage2D(type,
                         0,
                         format_to_gl_internal_format(desc.format),
                         desc.width, desc.height,
                         0,
                         format_to_gl_format(desc.format),
                         format_to_gl_data_type(desc.format),
                         nullptr);

            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(type, GL_TEXTURE_BORDER_COLOR, shadow_boarder);
        } break;
        case RT_SHADOW_CUBE_MAP: {
            type = GL_TEXTURE_CUBE_MAP;
            glBindTexture(type, texture_id);
            for (int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                             0,
                             format_to_gl_internal_format(desc.format),
                             desc.width, desc.height,
                             0,
                             format_to_gl_format(desc.format),
                             format_to_gl_data_type(desc.format),
                             nullptr);
            }
            glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        } break;
        default:
            CRASH_NOW();
            break;
    }
    glBindTexture(type, 0);
    return texture_id;
}

void RenderGraph::add_pass(RenderPassDesc& desc) {
    std::shared_ptr<RenderPass> render_pass = std::make_shared<RenderPassGL>();
    render_pass->create_internal(desc);
    m_render_passes.emplace_back(render_pass);

    const std::string& name = render_pass->m_name;
    DEV_ASSERT(m_render_pass_lookup.find(name) == m_render_pass_lookup.end());
    m_render_pass_lookup[name] = (int)m_render_passes.size() - 1;
}

std::shared_ptr<Resource> RenderGraph::create_resource(const ResourceDesc& desc) {
    DEV_ASSERT(m_resource_lookup.find(desc.name) == m_resource_lookup.end());
    std::shared_ptr<Resource> resource = std::make_shared<Resource>(desc);

    resource->m_handle = create_resource_impl(resource->m_desc);
    resource->m_resident_handle = glGetTextureHandleARB(resource->m_handle);
    glMakeTextureHandleResidentARB(resource->m_resident_handle);

    m_resource_lookup[resource->m_desc.name] = resource;
    return resource;
}

std::shared_ptr<Resource> RenderGraph::find_resouce(const std::string& name) const {
    auto it = m_resource_lookup.find(name);
    if (it == m_resource_lookup.end()) {
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<RenderPass> RenderGraph::find_pass(const std::string& name) const {
    auto it = m_render_pass_lookup.find(name);
    if (it == m_render_pass_lookup.end()) {
        return nullptr;
    }

    return m_render_passes[it->second];
}

void RenderGraph::compile() {
    const int num_passes = (int)m_render_passes.size();

    Graph graph(num_passes);

    for (int pass_index = 0; pass_index < num_passes; ++pass_index) {
        const std::shared_ptr<RenderPass>& pass = m_render_passes[pass_index];
        for (const std::string& input : pass->m_inputs) {
            auto it = m_render_pass_lookup.find(input);
            if (it == m_render_pass_lookup.end()) {
                CRASH_NOW_MSG(std::format("dependency '{}' not found", input));
            } else {
                graph.add_edge(it->second, pass_index);
            }
        }
    }

    if (graph.has_cycle()) {
        CRASH_NOW_MSG("render graph has cycle");
    }

    graph.remove_redundant();

    m_levels = graph.build_level();
    m_sorted_order.reserve(num_passes);
    for (const auto& level : m_levels) {
        for (int i : level) {
            m_sorted_order.emplace_back(i);
        }
    }

    for (int i = 1; i < (int)m_levels.size(); ++i) {
        for (int from : m_levels[i - 1]) {
            for (int to : m_levels[i]) {
                if (graph.has_edge(from, to)) {
                    const RenderPass* a = m_render_passes[from].get();
                    const RenderPass* b = m_render_passes[to].get();
                    LOG_VERBOSE("[render graph] dependency from '{}' to '{}'", a->get_name(), b->get_name());
                    m_links.push_back({ from, to });
                }
            }
        }
    }
}

void RenderGraph::execute() {
    for (int index : m_sorted_order) {
        m_render_passes[index]->execute();
    }
}

}  // namespace my::rg
