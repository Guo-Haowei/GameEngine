#include "renderer_panel.h"

#include <imgui/imgui_internal.h>

#include "engine/render_graph/render_graph_defines.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/renderer/path_tracer_render_system.h"
#include "engine/runtime/common_dvars.h"
#include "engine/scene/scene.h"

namespace my {

static void CollapseWindow(const std::string& p_window_name, std::function<void(void)>&& p_funcion) {
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                     ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                                     ImGuiTreeNodeFlags_FramePadding;
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx(p_window_name.c_str(), flags, "%s", p_window_name.c_str());
        ImGui::PopStyleVar();

        if (open) {
            p_funcion();
            ImGui::TreePop();
        }
    }
}

void RendererPanel::UpdateInternal(Scene&) {
    ImGui::Text("Debug");
    ImGui::Text("Frame rate:%.2f", ImGui::GetIO().Framerate);
    ImGui::Checkbox("show editor", (bool*)DVAR_GET_POINTER(show_editor));

    CollapseWindow("Shadow", []() {
        ImGui::Checkbox("debug", (bool*)DVAR_GET_POINTER(gfx_debug_shadow));
    });

    CollapseWindow("IBL", []() {
        ImGui::Checkbox("enable", (bool*)DVAR_GET_POINTER(gfx_enable_ibl));
    });

    CollapseWindow("Bloom", []() {
        ImGui::Checkbox("enable", (bool*)DVAR_GET_POINTER(gfx_enable_bloom));
        ImGui::DragFloat("threshold", (float*)DVAR_GET_POINTER(gfx_bloom_threshold), 0.01f, 0.0f, 3.0f);
    });

    CollapseWindow("SSAO", []() {
        ImGui::Checkbox("enable", (bool*)DVAR_GET_POINTER(gfx_ssao_enabled));
        ImGui::DragFloat("kernel radius", (float*)DVAR_GET_POINTER(gfx_ssao_radius), 0.01f, 0.0f, 5.0f);
    });

    CollapseWindow("Path Tracer", []() {
        auto& gm = IGraphicsManager::GetSingleton();
        int selected = (int)gm.GetActiveRenderGraphName();
        const int prev_selected = selected;
        for (int i = 0; i < std::to_underlying(RenderGraphName::COUNT); ++i) {
            const char* name = ToString(static_cast<RenderGraphName>(i));
            name = name ? name : "xxx";
            ImGui::RadioButton(name, &selected, i);
        }
        if (prev_selected != selected) {
            if (gm.SetActiveRenderGraph((RenderGraphName)selected)) {
                if (selected == (int)RenderGraphName::PATHTRACER) {
                    SetPathTracerMode(PathTracerMode::INTERACTIVE);
                } else {
                    SetPathTracerMode(PathTracerMode::NONE);
                }
            }
        }

        if (ImGui::Button("Generate BVH")) {
            DVAR_SET_BOOL(gfx_bvh_generate, true);
        }
        int bvh_level = DVAR_GET_INT(gfx_bvh_debug);
        // drawing too many bvh nodes causes performance issue,
        // set a maximum number
        if (ImGui::DragInt("Debug BVH", &bvh_level, 0.1f, -1, 10)) {
            DVAR_SET_INT(gfx_bvh_debug, bvh_level);
        }
    });
}

}  // namespace my
