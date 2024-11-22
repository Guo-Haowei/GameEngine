#pragma once
#include "scene/scene.h"
#include "shared/undo_command.h"

namespace my {

class EditorLayer;

enum CommandType : uint8_t {
    COMMAND_TYPE_ENTITY_TRANSLATE,
    COMMAND_TYPE_ENTITY_ROTATE,
    COMMAND_TYPE_ENTITY_SCALE,

    COMMAND_TYPE_ENTITY_CREATE,
    COMMAND_TYPE_ENTITY_REMOVE,

    COMMAND_TYPE_COMPONENT_ADD,

    COMMAND_TYPE_SAVE_PROJECT,
    COMMAND_TYPE_REDO_VIEWER,
    COMMAND_TYPE_UNDO_VIEWER,
};

// clang-format off
//------------ Enum,                Name,           Separator
#define ENTITY_TYPE_LIST                                   \
    ENTITY_TYPE(INFINITE_LIGHT,		InfiniteLight,  false) \
    ENTITY_TYPE(POINT_LIGHT,		PointLight,     false) \
    ENTITY_TYPE(AREA_LIGHT,			AreaLight,      true ) \
    ENTITY_TYPE(TRANSFORM,			Transform,      false) \
    ENTITY_TYPE(PLANE,				Plane,          false) \
    ENTITY_TYPE(CUBE,				Cube,           false) \
    ENTITY_TYPE(SPHERE,             Sphere,         false) \
    ENTITY_TYPE(CYLINDER,           Cylinder,       false) \
    ENTITY_TYPE(TORUS,              Torus,          true ) \
    ENTITY_TYPE(PARTICLE_EMITTER,   Emitter,        false) \
    ENTITY_TYPE(FORCE_FIELD,        ForceField,     false)
// clang-format on

enum class EntityType : uint8_t {
#define ENTITY_TYPE(ENUM, ...) ENUM,
    ENTITY_TYPE_LIST
#undef ENTITY_TYPE
        COUNT,
};

enum class ComponentType : uint8_t {
    BOX_COLLIDER,
    MESH_COLLIDER,
    SPHERE_COLLIDER,
};

// @TODO: make editor UndoCommand and editor Command
// struct EditorCommandBase {
//    EditorLayer* m_editor{ nullptr };
//};
// @REMOVE DO from ICommand;

// @TODO: make proctected
class EditorCommandAddEntity : public ICommand {
public:
    EditorCommandAddEntity(EntityType p_entity_type)
        : ICommand(COMMAND_TYPE_ENTITY_CREATE), entityType(p_entity_type) {}

    EntityType entityType;
    ecs::Entity parent;
    ecs::Entity entity;
};

class EditorCommandAddComponent : public ICommand {
public:
    EditorCommandAddComponent(ComponentType p_component_type)
        : ICommand(COMMAND_TYPE_COMPONENT_ADD), componentType(p_component_type) {}

    ComponentType componentType;
    ecs::Entity target;
};

class EditorCommandRemoveEntity : public ICommand {
public:
    EditorCommandRemoveEntity(ecs::Entity p_target)
        : ICommand(COMMAND_TYPE_ENTITY_REMOVE), target(p_target) {}

    ecs::Entity target;
};

class EntityTransformCommand : public UndoCommand {
public:
    EntityTransformCommand(CommandType p_type,
                           Scene& p_scene,
                           ecs::Entity p_entity,
                           const mat4& p_before,
                           const mat4& p_after);

    void Undo() override;
    void Redo() override;

    bool MergeCommand(const ICommand* p_command) override;

protected:
    Scene& m_scene;
    ecs::Entity m_entity;

    mat4 m_before;
    mat4 m_after;
};

class SaveProjectCommand : public ICommand {
public:
    SaveProjectCommand(bool p_open_dialog) : ICommand(COMMAND_TYPE_SAVE_PROJECT), m_openDialog(p_open_dialog) {}

    virtual void Redo() override;

protected:
    bool m_openDialog;
};

class UndoViewerCommand : public ICommand {
public:
    UndoViewerCommand(EditorLayer& p_editor) : ICommand(COMMAND_TYPE_UNDO_VIEWER), m_editor(p_editor) {}

    virtual void Redo() override;

protected:
    EditorLayer& m_editor;
};

class RedoViewerCommand : public ICommand {
public:
    RedoViewerCommand(EditorLayer& p_editor) : ICommand(COMMAND_TYPE_REDO_VIEWER), m_editor(p_editor) {}

    virtual void Redo() override;

protected:
    EditorLayer& m_editor;
};

}  // namespace my
