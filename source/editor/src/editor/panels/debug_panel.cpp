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

#if 0
template<typename T, typename UIFunction>
static void DrawComponent(const std::string& name, T* component, UIFunction uiFunction) {
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                             ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                                             ImGuiTreeNodeFlags_FramePadding;
    if (component) {
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImGui::Separator();
        bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
        ImGui::PopStyleVar();
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("-", ImVec2{ lineHeight, lineHeight })) {
            ImGui::OpenPopup("ComponentSettings");
        }

        bool removeComponent = false;
        if (ImGui::BeginPopup("ComponentSettings")) {
            if (ImGui::MenuItem("remove component")) {
                removeComponent = true;
            }

            ImGui::EndPopup();
        }

        if (open) {
            uiFunction(*component);
            ImGui::TreePop();
        }

        if (removeComponent) {
            LOG_ERROR("TODO: implement remove component");
        }
    }
}
#endif

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

    dvar_checkbox(DVAR_r_enable_bloom);
    ImGui::DragInt("Bloom downsample", (int*)DVAR_GET_POINTER(r_debug_bloom_downsample), 0.1f, 0, BLOOM_MIP_CHAIN_MAX - 1);
    ImGui::DragFloat("Bloom threshold", (float*)DVAR_GET_POINTER(r_bloom_threshold), 0.1f, 0.0f, 5.0f);

    ImGui::Separator();
}

}  // namespace my
