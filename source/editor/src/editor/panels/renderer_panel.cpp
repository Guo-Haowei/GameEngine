#include "renderer_panel.h"

#include <imgui/imgui_internal.h>

#include "core/framework/common_dvars.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/rendering_dvars.h"
#include "scene/scene.h"

namespace my {

#if 0
static void disable_widget() {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
}

static void enable_widget() {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
}
#endif

static void collapse_window(const std::string& p_window_name, std::function<void(void)>&& p_funcion) {
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                     ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                                     ImGuiTreeNodeFlags_FramePadding;
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx(p_window_name.c_str(), flags, p_window_name.c_str());
        ImGui::PopStyleVar();

        if (open) {
            p_funcion();
            ImGui::TreePop();
        }
    }
}

void RendererPanel::update_internal(Scene&) {
    ImGui::Text("Debug");
    ImGui::Text("Frame rate:%.2f", ImGui::GetIO().Framerate);
    ImGui::Checkbox("show editor", (bool*)DVAR_GET_POINTER(show_editor));
    ImGui::Checkbox("no texture", (bool*)DVAR_GET_POINTER(r_no_texture));

    collapse_window("VXGI", []() {
        ImGui::Checkbox("enable", (bool*)DVAR_GET_POINTER(r_enable_vxgi));
        ImGui::Checkbox("debug", (bool*)DVAR_GET_POINTER(r_debug_vxgi));
        int value = DVAR_GET_INT(r_debug_vxgi_voxel);
        ImGui::RadioButton("lighting", &value, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Normal", &value, 1);
        DVAR_SET_INT(r_debug_vxgi_voxel, value);
    });

    collapse_window("SSAO", []() {
        ImGui::Checkbox("enable", (bool*)DVAR_GET_POINTER(r_enable_ssao));
        ImGui::SliderFloat("radius", (float*)(DVAR_GET_POINTER(r_ssao_kernel_radius)), 0.1f, 5.0f);
    });

    collapse_window("CSM", []() {
        ImGui::Checkbox("enable", (bool*)DVAR_GET_POINTER(r_enable_csm));
        ImGui::Checkbox("debug", (bool*)DVAR_GET_POINTER(r_debug_csm));
    });

    collapse_window("Bloom", []() {
        ImGui::Checkbox("enable", (bool*)DVAR_GET_POINTER(r_enable_bloom));
        ImGui::DragFloat("threshold", (float*)DVAR_GET_POINTER(r_bloom_threshold), 0.01f, 0.0f, 3.0f);
        // ImGui::DragInt("Bloom downsample", (int*)DVAR_GET_POINTER(r_debug_bloom_downsample), 0.1f, 0, BLOOM_MIP_CHAIN_MAX - 1);
    });
}

}  // namespace my
