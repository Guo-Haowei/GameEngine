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
    AddPanel(std::make_shared<RenderGraphEditor>(*this));
    AddPanel(std::make_shared<ConsolePanel>(*this));
    AddPanel(std::make_shared<RendererPanel>(*this));
    AddPanel(std::make_shared<HierarchyPanel>(*this));
    AddPanel(std::make_shared<PropertyPanel>(*this));
    AddPanel(std::make_shared<Viewer>(*this));
    AddPanel(std::make_shared<ContentBrowser>(*this));

    m_menuBar = std::make_shared<MenuBar>(*this);

    //// load assets
    // const char* light_icons[] = {
    //     "@res://images/arealight.png",
    //     "@res://images/pointlight.png",
    //     "@res://images/infinitelight.png",
    // };

    // for (int i = 0; i < array_length(light_icons); ++i) {
    //     AssetManager::singleton().load_image_sync(light_icons[i]);
    // }
}

void EditorLayer::AddPanel(std::shared_ptr<EditorItem> p_panel) {
    m_panels.emplace_back(p_panel);
}

void EditorLayer::SelectEntity(ecs::Entity p_selected) {
    m_selected = p_selected;
    // TODO: fix this, shouldn't fetch globally
    SceneManager::getScene().m_selected = m_selected;
}

// @TODO: make this an item
void EditorLayer::DockSpace(Scene& p_scene) {
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

    m_menuBar->Update(p_scene);

    ImGui::End();
    return;
}

void EditorLayer::Update(float) {
    Scene& scene = SceneManager::getScene();
    DockSpace(scene);
    for (auto& it : m_panels) {
        it->Update(scene);
    }
    FlushCommand(scene);
}

void EditorLayer::Render() {
}

void EditorLayer::AddComponent(ComponentType p_type, ecs::Entity p_target) {
    auto command = std::make_shared<EditorCommandAddComponent>(p_type);
    command->target = p_target;
    BufferCommand(command);
}

void EditorLayer::AddEntity(EntityType p_name, ecs::Entity p_parent) {
    auto command = std::make_shared<EditorCommandAddEntity>(p_name);
    command->parent = p_parent;
    BufferCommand(command);
}

void EditorLayer::RemoveEntity(ecs::Entity p_target) {
    auto command = std::make_shared<EditorCommandRemoveEntity>(p_target);
    BufferCommand(command);
}

void EditorLayer::BufferCommand(std::shared_ptr<EditorCommand> p_command) {
    m_commandBuffer.emplace_back(std::move(p_command));
}

static std::string gen_name(std::string_view p_name) {
    static int s_counter = 0;
    return std::format("{}-{}", p_name, ++s_counter);
}

void EditorLayer::FlushCommand(Scene& scene) {
    while (!m_commandBuffer.empty()) {
        auto task = m_commandBuffer.front();
        m_commandBuffer.pop_front();
        do {
            if (auto add_command = dynamic_cast<EditorCommandAddEntity*>(task.get()); add_command) {
                ecs::Entity id;
                switch (add_command->entityType) {
                    case ENTITY_TYPE_INFINITE_LIGHT:
                        id = scene.CreateInfiniteLightEntity(gen_name("directional-light"));
                        break;
                    case ENTITY_TYPE_POINT_LIGHT:
                        id = scene.CreatePointLightEntity(gen_name("point-light"), vec3(0, 1, 0));
                        break;
                    case ENTITY_TYPE_AREA_LIGHT:
                        id = scene.CreateAreaLightEntity(gen_name("area-light"));
                        break;
                    case ENTITY_TYPE_PLANE:
                        id = scene.CreatePlaneEntity(gen_name("plane"));
                        break;
                    case ENTITY_TYPE_CUBE:
                        id = scene.CreateCubeEntity(gen_name("cube"));
                        break;
                    case ENTITY_TYPE_SPHERE:
                        id = scene.CreateSphereEntity(gen_name("sphere"));
                        break;
                    default:
                        CRASH_NOW();
                        break;
                }

                scene.AttachComponent(id, add_command->parent.IsValid() ? add_command->parent : scene.m_root);
                SelectEntity(id);
                SceneManager::singleton().bumpRevision();
                break;
            }
            if (auto command = dynamic_cast<EditorCommandAddComponent*>(task.get()); command) {
                DEV_ASSERT(command->target.IsValid());
                switch (command->componentType) {
                    case COMPONENT_TYPE_BOX_COLLIDER: {
                        auto& collider = scene.Create<BoxColliderComponent>(command->target);
                        collider.box = AABB::fromCenterSize(vec3(0), vec3(1));
                    } break;
                    case COMPONENT_TYPE_MESH_COLLIDER:
                        scene.Create<MeshColliderComponent>(command->target);
                        break;
                    default:
                        CRASH_NOW();
                        break;
                }
                break;
            }
            if (auto command = dynamic_cast<EditorCommandRemoveEntity*>(task.get()); command) {
                auto entity = command->target;
                DEV_ASSERT(entity.IsValid());
                scene.RemoveEntity(entity);
                // if (scene.contains<TransformComponent>(entity)) {

                //}
                break;
            }
        } while (0);
        m_commandHistory.push_back(task);
    }
}

void EditorLayer::UndoCommand(Scene&) {
    CRASH_NOW();
}

}  // namespace my
