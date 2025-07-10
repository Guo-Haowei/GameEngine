#include "menu_bar.h"

#include "editor/editor_layer.h"
#include "editor/widget.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/runtime/input_manager.h"

namespace my {

void MenuBar::MainMenuBar() {
    const auto& shortcuts = m_editor.GetShortcuts();
    auto build_menu_item = [&](int p_index) {
        const auto& it = shortcuts[p_index];
        const bool enabled = it.enabledFunc ? it.enabledFunc() : true;
        if (ImGui::MenuItem(it.name, it.shortcut, false, enabled)) {
            it.executeFunc();
        }
    };

    if (ImGui::BeginMenu("File")) {
        build_menu_item(SHORT_CUT_OPEN);
        // Open Recent
        ImGui::Separator();
        build_menu_item(SHORT_CUT_SAVE);
        build_menu_item(SHORT_CUT_SAVE_AS);
        ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Edit")) {
        build_menu_item(SHORT_CUT_UNDO);
        build_menu_item(SHORT_CUT_REDO);
        ImGui::Separator();
        if (ImGui::MenuItem("Cut", "Ctrl+X")) {
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C")) {
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();
    EditorItem::OpenAddEntityPopup(ecs::Entity::INVALID);
}

void MenuBar::Update(Scene&) {

    #if 0
    if (ImGui::BeginMenuBar()) {
        ViewerBar();
        ImGui::EndMenuBar();
    }

    ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();

    if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags)) {
        if (ImGui::BeginMenuBar()) {
            MainMenuBar();
            ImGui::EndMenuBar();
        }
        ImGui::End();
    }
#endif

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetCurrentViewport(nullptr, (ImGuiViewportP*)viewport);  // Set viewport explicitly so GetFrameHeight reacts to DPI changes

    float height = ImGui::GetFrameHeight();

    if (ImGui::BeginMainMenuBar()) {
        MainMenuBar();
        ImGui::EndMainMenuBar();
    }

    if (ImGui::BeginViewportSideBar("StatusBar", viewport, ImGuiDir_Down, height, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            ImGui::Text("status bar");
            ImGui::EndMenuBar();
        }
        //if (ImGui::BeginMenuBar()) {
        //    ViewerBar();
        //    ImGui::EndMenuBar();
        //}
        ImGui::End();
    }
}

void MenuBar::ViewerBar() {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImVec2 contentSize(200.0f, 100.0f);  // You may calculate this dynamically

    ImVec2 offset((avail.x - contentSize.x) * 0.5f, 10.0f);
    ImGui::SetCursorPos(offset);

    ImGui::BeginGroup(); 
    ImGui::Text("Centered Content");
    ImGui::Button("Click Me");

    static bool ok = true;
    ToggleButton("dummy", &ok);
    ImGui::EndGroup();
}

}  // namespace my
