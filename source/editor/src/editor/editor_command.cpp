#include "editor_command.h"

#include "core/framework/common_dvars.h"
#include "drivers/windows/dialog.h"
#include "editor_layer.h"
// @TODO: refactor
#include "core/framework/scene_manager.h"

namespace my {

/// TransformCommand
EntityTransformCommand::EntityTransformCommand(CommandType p_type,
                                               Scene& p_scene,
                                               ecs::Entity p_entity,
                                               const mat4& p_before,
                                               const mat4& p_after) : UndoCommand(p_type),
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

bool EntityTransformCommand::MergeCommand(const ICommand* p_command) {
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

/// SaveProjectCommand
void SaveProjectCommand::Redo() {
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

    DVAR_SET_STRING(project, path.string());
    Scene& scene = SceneManager::GetSingleton().GetScene();

    Archive archive;
    if (!archive.OpenWrite(path.string())) {
        return;
    }

    if (scene.Serialize(archive)) {
        LOG_OK("scene saved to '{}'", path.string());
    }
}

/// RedoViewerCommand
void RedoViewerCommand::Redo() {
    m_editor.GetUndoStack().Redo();
}

/// UndoViewerCommand
void UndoViewerCommand::Redo() {
    m_editor.GetUndoStack().Undo();
}

}  // namespace my
