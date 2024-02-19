#include "editor_layer.h"

#include "imgui/imgui_internal.h"
#include "rendering/r_cbuffers.h"
/////////////////////
#include "core/framework/asset_manager.h"
#include "core/framework/scene_manager.h"
#include "core/input/input.h"
#include "editor/panels/animation_panel.h"
#include "editor/panels/console_panel.h"
#include "editor/panels/debug_panel.h"
#include "editor/panels/debug_texture.h"
#include "editor/panels/hierarchy_panel.h"
#include "editor/panels/propertiy_panel.h"
#include "editor/panels/render_graph_editor.h"
#include "editor/panels/viewer.h"
#include "servers/display_server.h"

namespace my {

EditorLayer::EditorLayer() : Layer("EditorLayer") {
    add_panel(std::make_shared<RenderGraphEditor>(*this));
    add_panel(std::make_shared<AnimationPanel>(*this));
    add_panel(std::make_shared<ConsolePanel>(*this));
    add_panel(std::make_shared<DebugPanel>(*this));
    add_panel(std::make_shared<DebugTexturePanel>(*this));
    add_panel(std::make_shared<HierarchyPanel>(*this));
    add_panel(std::make_shared<PropertyPanel>(*this));
    add_panel(std::make_shared<Viewer>(*this));

    m_menu_bar = std::make_shared<MenuBar>(*this);

    // load assets
    const char* light_icons[] = {
        "@res://images/arealight.png",
        "@res://images/pointlight.png",
        "@res://images/omnilight.png",
    };

    for (int i = 0; i < array_length(light_icons); ++i) {
        AssetManager::singleton().load_image_sync(light_icons[i]);
    }
}

void EditorLayer::add_panel(std::shared_ptr<EditorWindow> panel) {
    m_panels.emplace_back(panel);
}

void EditorLayer::select_entity(ecs::Entity selected) {
    m_selected = selected;
    m_state = STATE_PICKING;
}

void EditorLayer::dock_space(Scene& scene) {
    ImGui::GetMainViewport();

    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
        window_flags |= ImGuiWindowFlags_NoBackground;
    }

    if (!opt_padding) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding) {
        ImGui::PopStyleVar();
    }

    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    m_menu_bar->draw(scene);

    ImGui::End();
    return;
}

void EditorLayer::update(float) {
    Scene& scene = SceneManager::get_scene();
    dock_space(scene);
    for (auto& it : m_panels) {
        it->update(scene);
    }
    flush_commands(scene);
}

void EditorLayer::render() {
}

void EditorLayer::add_object(EditorCommandName name, ecs::Entity parent) {
    auto command = std::make_shared<EditorCommandAdd>(name);
    command->parent = parent;
    buffer_command(command);
}

void EditorLayer::add_plane(ecs::Entity parent) {
    add_object(EDITOR_COMMAND_ADD_MESH_PLANE, parent);
}

void EditorLayer::add_cube(ecs::Entity parent) {
    add_object(EDITOR_COMMAND_ADD_MESH_CUBE, parent);
}

void EditorLayer::add_sphere(ecs::Entity parent) {
    add_object(EDITOR_COMMAND_ADD_MESH_SPHERE, parent);
}

void EditorLayer::add_point_light(ecs::Entity parent) {
    add_object(EDITOR_COMMAND_ADD_LIGHT_POINT, parent);
}

void EditorLayer::add_omin_light(ecs::Entity parent) {
    add_object(EDITOR_COMMAND_ADD_LIGHT_OMNI, parent);
}

void EditorLayer::buffer_command(std::shared_ptr<EditorCommand> command) {
    m_command_buffer.emplace_back(std::move(command));
}

static std::string gen_name(std::string_view name) {
    static int s_counter = 0;
    return std::format("{}_{}", name, ++s_counter);
}

void EditorLayer::flush_commands(Scene& scene) {
    while (!m_command_buffer.empty()) {
        auto command = m_command_buffer.front();
        m_command_buffer.pop_front();
        do {
            EditorCommandAdd* add_command = dynamic_cast<EditorCommandAdd*>(command.get());
            if (add_command) {
                if (!add_command->parent.is_valid()) {
                    add_command->parent == scene.m_root;
                }

                ecs::Entity id;
                switch (add_command->name) {
                    case EDITOR_COMMAND_ADD_LIGHT_OMNI:
                        id = scene.create_omnilight_entity(gen_name("OmniLight"));
                        break;
                    case EDITOR_COMMAND_ADD_LIGHT_POINT:
                        id = scene.create_pointlight_entity(gen_name("Pointlight"), vec3(0, 1, 0));
                        break;
                    case EDITOR_COMMAND_ADD_MESH_PLANE:
                        id = scene.create_cube_entity(gen_name("Plane"));
                        break;
                    case EDITOR_COMMAND_ADD_MESH_CUBE:
                        id = scene.create_cube_entity(gen_name("Cube"));
                        break;
                    case EDITOR_COMMAND_ADD_MESH_SPHERE:
                        id = scene.create_sphere_entity(gen_name("Sphere"));
                        break;
                    default:
                        CRASH_NOW();
                        break;
                }

                scene.attach_component(id, add_command->parent);
                select_entity(id);
                SceneManager::singleton().bump_revision();
                break;
            }
        } while (0);
        m_command_history.push_back(command);
    }
}

void EditorLayer::undo_command(Scene&) {
    CRASH_NOW();
}

}  // namespace my
