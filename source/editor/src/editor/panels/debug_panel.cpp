#include "debug_panel.h"

#include <imgui/imgui_internal.h>

#include "core/framework/common_dvars.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/rendering_dvars.h"
#include "scene/scene.h"

namespace my {

using CheckBoxFunc = void (*)(void);

// @TODO: move it some where
static void disable_widget() {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
}

static void enable_widget() {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
}

static void dvar_checkbox(DynamicVariable& dvar, CheckBoxFunc func = nullptr) {
    ImGui::Checkbox(dvar.get_desc(), (bool*)dvar.as_pointer());
    if (func) {
        const bool disabled = !dvar.as_int();
        if (disabled) {
            disable_widget();
        }

        func();

        if (disabled) {
            enable_widget();
        }
    }
    ImGui::Separator();
}

void DebugPanel::update_internal(Scene&) {
    ImGui::Text("Debug");
    ImGui::Text("Frame rate:%.2f", ImGui::GetIO().Framerate);

    dvar_checkbox(DVAR_r_enable_vxgi);
    dvar_checkbox(DVAR_r_no_texture);
    dvar_checkbox(DVAR_r_debug_vxgi, []() {
        ImGui::Text("Display voxel");
        int value = DVAR_GET_INT(r_debug_vxgi_voxel);
        ImGui::RadioButton("lighting", &value, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Normal", &value, 1);
        DVAR_SET_INT(r_debug_vxgi_voxel, value);
    });

    dvar_checkbox(DVAR_r_enable_ssao, []() {
        ImGui::Text("SSAO Kernal Radius");
        ImGui::SliderFloat("Kernal Radius", (float*)(DVAR_GET_POINTER(r_ssaoKernelRadius)), 0.1f, 5.0f);
    });

    dvar_checkbox(DVAR_r_enable_csm, []() {
        ImGui::Checkbox("Debug CSM", (bool*)DVAR_GET_POINTER(r_debug_csm));
    });
    dvar_checkbox(DVAR_show_editor);

    ImGui::DragInt("Bloom downsample", (int*)DVAR_GET_POINTER(r_debug_bloom_downsample), 0.1f, 0, BLOOM_MIP_CHAIN_MAX - 1);

    ImGui::Separator();
}

}  // namespace my
