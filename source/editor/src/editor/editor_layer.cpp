#include "editor_layer.h"

#include <imgui/imgui_internal.h>

#include "core/framework/asset_manager.h"
#include "core/framework/scene_manager.h"
#include "core/input/input.h"
#include "editor/panels/console_panel.h"
#include "editor/panels/content_browser.h"
#include "editor/panels/hierarchy_panel.h"
#include "editor/panels/propertiy_panel.h"
#include "editor/panels/render_graph_editor.h"
#include "editor/panels/renderer_panel.h"
#include "editor/panels/viewer.h"

namespace my {

EditorLayer::EditorLayer() : Layer("EditorLayer") {
    add_panel(std::make_shared<RenderGraphEditor>(*this));
    add_panel(std::make_shared<ConsolePanel>(*this));
    add_panel(std::make_shared<RendererPanel>(*this));
    add_panel(std::make_shared<HierarchyPanel>(*this));
    add_panel(std::make_shared<PropertyPanel>(*this));
    add_panel(std::make_shared<Viewer>(*this));
    add_panel(std::make_shared<ContentBrowser>(*this));

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

void EditorLayer::add_panel(std::shared_ptr<EditorItem> p_panel) {
    m_panels.emplace_back(p_panel);
}

void EditorLayer::select_entity(ecs::Entity p_selected) {
    m_selected = p_selected;
}

// @TODO: make this an item
void EditorLayer::dock_space(Scene& p_scene) {
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

    m_menu_bar->update(p_scene);

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

void EditorLayer::add_component(ComponentType p_type, ecs::Entity p_target) {
    auto command = std::make_shared<EditorCommandAddComponent>(p_type);
    command->target = p_target;
    buffer_command(command);
}

void EditorLayer::add_entity(EntityType p_name, ecs::Entity p_parent) {
    auto command = std::make_shared<EditorCommandAddEntity>(p_name);
    command->parent = p_parent;
    buffer_command(command);
}

void EditorLayer::remove_entity(ecs::Entity p_target) {
    auto command = std::make_shared<EditorCommandRemoveEntity>(p_target);
    buffer_command(command);
}

void EditorLayer::buffer_command(std::shared_ptr<EditorCommand> p_command) {
    m_command_buffer.emplace_back(std::move(p_command));
}

static std::string gen_name(std::string_view p_name) {
    static int s_counter = 0;
    return std::format("{}-{}", p_name, ++s_counter);
}

void EditorLayer::flush_commands(Scene& scene) {
    while (!m_command_buffer.empty()) {
        auto task = m_command_buffer.front();
        m_command_buffer.pop_front();
        do {
            if (auto add_command = dynamic_cast<EditorCommandAddEntity*>(task.get()); add_command) {
                ecs::Entity id;
                switch (add_command->entity_type) {
                    case ENTITY_TYPE_OMNI_LIGHT:
                        id = scene.create_omni_light_entity(gen_name("directional-light"));
                        break;
                    case ENTITY_TYPE_POINT_LIGHT:
                        id = scene.create_point_light_entity(gen_name("point-light"), vec3(0, 1, 0));
                        break;
                    case ENTITY_TYPE_AREA_LIGHT:
                        id = scene.create_area_light_entity(gen_name("area-light"));
                        break;
                    case ENTITY_TYPE_PLANE:
                        id = scene.create_plane_entity(gen_name("plane"));
                        break;
                    case ENTITY_TYPE_CUBE:
                        id = scene.create_cube_entity(gen_name("cube"));
                        break;
                    case ENTITY_TYPE_SPHERE:
                        id = scene.create_sphere_entity(gen_name("sphere"));
                        break;
                    default:
                        CRASH_NOW();
                        break;
                }

                scene.attach_component(id, add_command->parent.is_valid() ? add_command->parent : scene.m_root);
                select_entity(id);
                SceneManager::singleton().bump_revision();
                break;
            }
            if (auto command = dynamic_cast<EditorCommandAddComponent*>(task.get()); command) {
                DEV_ASSERT(command->target.is_valid());
                switch (command->component_type) {
                    case COMPONENT_TYPE_BOX_COLLIDER: {
                        auto& collider = scene.create<BoxColliderComponent>(command->target);
                        collider.box = AABB::from_center_size(vec3(0), vec3(1));
                    } break;
                    case COMPONENT_TYPE_MESH_COLLIDER:
                        scene.create<MeshColliderComponent>(command->target);
                        break;
                    default:
                        CRASH_NOW();
                        break;
                }
                break;
            }
            if (auto command = dynamic_cast<EditorCommandRemoveEntity*>(task.get()); command) {
                auto entity = command->target;
                DEV_ASSERT(entity.is_valid());
                scene.remove_entity(entity);
                // if (scene.contains<TransformComponent>(entity)) {

                //}
                break;
            }
        } while (0);
        m_command_history.push_back(task);
    }
}

void EditorLayer::undo_command(Scene&) {
    CRASH_NOW();
}

}  // namespace my
