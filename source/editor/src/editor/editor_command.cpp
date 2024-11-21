#include "editor_command.h"

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
    // @TODO: DEV_VERIFY
    DEV_ASSERT(transform);
    transform->SetLocalTransform(m_before);
}

void EntityTransformCommand::Redo() {
    TransformComponent* transform = m_scene.GetComponent<TransformComponent>(m_entity);
    DEV_ASSERT(transform);
    transform->SetLocalTransform(m_after);
}

bool EntityTransformCommand::MergeCommand(const ICommand* p_command) {
    unused(p_command);
    CRASH_NOW();
}

}  // namespace my
