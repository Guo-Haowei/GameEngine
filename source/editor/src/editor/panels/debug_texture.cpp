#include "debug_texture.h"

#include "core/framework/graphics_manager.h"
#include "editor/editor_layer.h"

namespace my {

void DebugTexturePanel::update_internal(Scene& scene) {
    unused(scene);

    const auto& graph = GraphicsManager::singleton().get_active_render_graph();
    const auto& resources = graph.get_resources();

    std::vector<const char*> items;

    static int selected = 0;
    for (const auto& it : resources) {
        items.push_back(it.first.c_str());
    }

    ImGui::Combo("Debug texture", &selected, items.data(), (int)items.size());

    std::string key = items[selected];
    const auto& it = resources.find(key);
    const auto resource = it->second;
    const auto& desc = resource->get_desc();
    uint64_t handle = resource->get_handle();

    vec2 dim{ desc.width, desc.height };
    dim *= 0.5f;
    ImGui::Image((ImTextureID)handle, ImVec2(dim.x, dim.y), ImVec2(0, 1), ImVec2(1, 0));

    ImGui::Separator();
}

}  // namespace my
