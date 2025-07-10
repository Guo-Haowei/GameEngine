#include "tool_bar.h"

#include "editor/editor_layer.h"
#include "editor/widget.h"

namespace my {

void ToolBar::Update(Scene&) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    auto& colors = ImGui::GetStyle().Colors;
    const auto& button_hovered = colors[ImGuiCol_ButtonHovered];
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(button_hovered.x, button_hovered.y, button_hovered.z, 0.5f));
    const auto& button_active = colors[ImGuiCol_ButtonActive];
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(button_active.x, button_active.y, button_active.z, 0.5f));

    ImGuiWindowFlags toolbar_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoDocking;
#if 0
    toolbar_flags |= ImGuiWindowFlags_NoDecoration;
#endif
    ImGui::Begin("##toolbar", nullptr, toolbar_flags);

    bool toolbar_enabled = true;
    ImVec4 tint_color = ImVec4(1, 1, 1, 1);
    if (!toolbar_enabled) {
        tint_color.w = 0.5f;
    }

    float size = ImGui::GetWindowHeight() - 4.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

    auto& context = m_editor.context;
    auto app = m_editor.GetApplication();
    auto app_state = app->GetState();

    if (auto image = context.playButtonImage; image && image->gpu_texture) {
        ImVec2 image_size(static_cast<float>(image->width), static_cast<float>(image->height));
        bool disable = app_state != Application::State::EDITING;
        ImGui::BeginDisabled(disable);
        if (ImGui::ImageButton("play", (ImTextureID)image->gpu_texture->GetHandle(), image_size)) {
            m_editor.GetApplication()->SetState(Application::State::BEGIN_SIM);
        }
        ImGui::EndDisabled();
    }
    ImGui::SameLine();
    if (auto image = context.pauseButtonImage; image && image->gpu_texture) {
        ImVec2 image_size(static_cast<float>(image->width), static_cast<float>(image->height));
        bool disable = app_state != Application::State::SIM;
        ImGui::BeginDisabled(disable);
        if (ImGui::ImageButton("pause", (ImTextureID)image->gpu_texture->GetHandle(), image_size)) {
            m_editor.GetApplication()->SetState(Application::State::END_SIM);
        }
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    ImGui::Text("2D View");
    ImGui::SameLine();

    bool is_2d = context.cameraType == CAMERA_2D;
    ToggleButton("XXX", &is_2d);
    context.cameraType = is_2d ? CAMERA_2D : CAMERA_3D;

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    ImGui::End();
}

}  // namespace my
