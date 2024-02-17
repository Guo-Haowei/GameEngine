#include "debug_panel.h"

#include <imgui/imgui_internal.h>

#include "core/framework/common_dvars.h"
#include "rendering/r_defines.h"
#include "rendering/rendering_dvars.h"
#include "scene/scene.h"

namespace my {

using CheckBoxFunc = void (*)(void);

static void dvar_checkbox(DynamicVariable& dvar, CheckBoxFunc func = nullptr) {
    ImGui::Checkbox(dvar.get_desc(), (bool*)dvar.as_pointer());
    if (func) {
        const bool disabled = !dvar.as_int();
        if (disabled) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        func();

        if (disabled) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
    }
    ImGui::Separator();
}

void DebugPanel::update_internal(Scene&) {
    ImGui::Text("Graphics Debug");

    dvar_checkbox(DVAR_r_enable_vxgi);
    dvar_checkbox(DVAR_r_no_texture);

    int debug_texture = DVAR_GET_INT(r_debug_texture);
    constexpr float offset = 150;
    ImGui::RadioButton("VXGI", &debug_texture, 0);
    ImGui::SameLine(ImGui::GetWindowWidth() - offset);
    ImGui::RadioButton("Depth Map", &debug_texture, TEXTURE_GBUFFER_DEPTH);

    ImGui::RadioButton("Voxel: Color", &debug_texture, TEXTURE_VOXEL_ALBEDO);
    ImGui::SameLine(ImGui::GetWindowWidth() - offset);
    ImGui::RadioButton("Voxel: Normal", &debug_texture, TEXTURE_VOXEL_NORMAL);

    ImGui::RadioButton("Gbuffer: Albedo", &debug_texture, TEXTURE_GBUFFER_ALBEDO);
    ImGui::SameLine(ImGui::GetWindowWidth() - offset);
    ImGui::RadioButton("Gbuffer: Normal", &debug_texture, TEXTURE_GBUFFER_NORMAL);

    ImGui::RadioButton("Gbuffer: Roughness", &debug_texture, TEXTURE_GBUFFER_ROUGHNESS);
    ImGui::SameLine(ImGui::GetWindowWidth() - offset);
    ImGui::RadioButton("Gbuffer: Metallic ", &debug_texture, TEXTURE_GBUFFER_METALLIC);

    ImGui::RadioButton("SSAO Map", &debug_texture, TEXTURE_SSAO);
    ImGui::SameLine(ImGui::GetWindowWidth() - offset);
    ImGui::RadioButton("Shadow Map", &debug_texture, TEXTURE_SHADOW_MAP);

    DVAR_SET_INT(r_debug_texture, debug_texture);
    ImGui::Separator();

    dvar_checkbox(DVAR_r_enable_ssao, []() {
        ImGui::Text("SSAO Kernal Radius");
        ImGui::SliderFloat("Kernal Radius", (float*)(DVAR_GET_POINTER(r_ssaoKernelRadius)), 0.1f, 5.0f);
    });

    dvar_checkbox(DVAR_r_debug_csm);
    dvar_checkbox(DVAR_r_enable_fxaa);
    dvar_checkbox(DVAR_grid_visibility);
}

}  // namespace my
