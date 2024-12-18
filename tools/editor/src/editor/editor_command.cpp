#include "editor_command.h"

#include "editor_layer.h"
#include "engine/core/framework/common_dvars.h"
#include "engine/core/string/string_utils.h"
#include "engine/drivers/windows/dialog.h"
#include "engine/scene/scene_serialization.h"
// @TODO: refactor
#include "engine/core/framework/scene_manager.h"

namespace my {

static std::string GenerateName(std::string_view p_name) {
    static int s_counter = 0;
    return std::format("{}-{}", p_name, ++s_counter);
}

/// EditorCommandAddEntity
void EditorCommandAddEntity::Execute(Scene& p_scene) {
    ecs::Entity id;
    switch (m_entityType) {
#define ENTITY_TYPE(ENUM, NAME, ...)                            \
    case EntityType::ENUM: {                                    \
        id = p_scene.Create##NAME##Entity(GenerateName(#NAME)); \
    } break;
        ENTITY_TYPE_LIST
#undef ENTITY_TYPE
        default:
            LOG_FATAL("Entity type {} not supported", static_cast<int>(m_entityType));
            break;
    }

    p_scene.AttachChild(id, m_parent.IsValid() ? m_parent : p_scene.m_root);
    m_editor->SelectEntity(id);
    SceneManager::GetSingleton().BumpRevision();
}

/// EditorCommandAddComponent
void EditorCommandAddComponent::Execute(Scene& p_scene) {
    DEV_ASSERT(target.IsValid());
    switch (m_componentType) {
        case ComponentType::BOX_COLLIDER: {
            auto& collider = p_scene.Create<BoxColliderComponent>(target);
            collider.box = AABB::FromCenterSize(Vector3f(0), Vector3f(1));
        } break;
        case ComponentType::MESH_COLLIDER: {
            p_scene.Create<MeshColliderComponent>(target);
        } break;
        case ComponentType::SCRIPT: {
            p_scene.Create<LuaScriptComponent>(target);
        } break;
        default: {
            CRASH_NOW();
        } break;
    }
}

/// EditorCommandRemoveEntity
void EditorCommandRemoveEntity::Execute(Scene& p_scene) {
    auto entity = m_target;
    DEV_ASSERT(entity.IsValid());
    p_scene.RemoveEntity(entity);
}

/// OpenProjectCommand
void OpenProjectCommand::Execute(Scene&) {
    std::string path;
    // std::filesystem::path path{ project.empty() ? "untitled.scene" : project.c_str() };
    if (m_openDialog) {
// @TODO: implement
#if USING(PLATFORM_WINDOWS)
        path = OpenFileDialog({});
        // @TODO: validate string
#else
        LOG_WARN("OpenSaveDialog not implemented");
#endif
    }

    // @TODO: validate
    DVAR_SET_STRING(project, path);

    SceneManager::GetSingleton().RequestScene(path);
}

/// SaveProjectCommand
void SaveProjectCommand::Execute(Scene& p_scene) {
    const std::string& project = DVAR_GET_STRING(project);

    std::filesystem::path path{ project.empty() ? "untitled.scene" : project.c_str() };
    if (m_openDialog || project.empty()) {
// @TODO: implement
#if USING(PLATFORM_WINDOWS)
        if (!OpenSaveDialog(path)) {
            return;
        }
#else
        LOG_WARN("OpenSaveDialog not implemented");
#endif
    }

    auto path_string = path.string();
    DVAR_SET_STRING(project, path_string);

    const auto extension = StringUtils::Extension(path_string);
    if (extension == ".scene") {
        if (auto res = SaveSceneBinary(path_string, p_scene); !res) {
            CRASH_NOW();
        }
    } else if (extension == ".yaml") {
        if (auto res = SaveSceneText(path_string, p_scene); !res) {
            CRASH_NOW();
        }
    }

    LOG_OK("scene saved to '{}'", path.string());
}

/// RedoViewerCommand
void RedoViewerCommand::Execute(Scene&) {
    m_editor->GetUndoStack().Redo();
}

/// UndoViewerCommand
void UndoViewerCommand::Execute(Scene&) {
    m_editor->GetUndoStack().Undo();
}

/// TransformCommand
EntityTransformCommand::EntityTransformCommand(CommandType p_type,
                                               Scene& p_scene,
                                               ecs::Entity p_entity,
                                               const Matrix4x4f& p_before,
                                               const Matrix4x4f& p_after) : EditorUndoCommandBase(p_type),
                                                                            m_scene(p_scene),
                                                                            m_entity(p_entity),
                                                                            m_before(p_before),
                                                                            m_after(p_after) {
}

void EntityTransformCommand::Undo() {
    TransformComponent* transform = m_scene.GetComponent<TransformComponent>(m_entity);
    if (DEV_VERIFY(transform)) {
        transform->SetLocalTransform(m_before);
    }
}

void EntityTransformCommand::Redo() {
    TransformComponent* transform = m_scene.GetComponent<TransformComponent>(m_entity);
    if (DEV_VERIFY(transform)) {
        transform->SetLocalTransform(m_after);
    }
}

bool EntityTransformCommand::MergeCommand(const UndoCommand* p_command) {
    auto command = dynamic_cast<const EntityTransformCommand*>(p_command);
    if (!command) {
        return false;
    }

    if (command->m_entity != m_entity) {
        return false;
    }

    if (command->m_type != m_type) {
        return false;
    }

    m_after = command->m_after;
    return true;
}

}  // namespace my
