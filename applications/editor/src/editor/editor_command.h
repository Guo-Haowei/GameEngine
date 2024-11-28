#pragma once
#include "engine/scene/scene.h"
#include "engine/shared/undo_command.h"

namespace my {

class EditorLayer;

enum CommandType : uint8_t {
    COMMAND_TYPE_ENTITY_TRANSLATE,
    COMMAND_TYPE_ENTITY_ROTATE,
    COMMAND_TYPE_ENTITY_SCALE,

    COMMAND_TYPE_ENTITY_CREATE,
    COMMAND_TYPE_ENTITY_REMOVE,

    COMMAND_TYPE_COMPONENT_ADD,

    COMMAND_TYPE_OPEN_PROJECT,
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

class EditorCommandBase {
public:
    EditorCommandBase(CommandType p_type) : m_type(p_type) {}

    virtual ~EditorCommandBase() = default;

    virtual void Execute(Scene& p_scene) = 0;

protected:
    EditorLayer* m_editor{ nullptr };
    const CommandType m_type;

    friend class EditorLayer;
};

class EditorUndoCommandBase : public EditorCommandBase, public UndoCommand {
public:
    EditorUndoCommandBase(CommandType p_type) : EditorCommandBase(p_type) {}

    void Execute(Scene&) final { Redo(); }
};

class EditorCommandAddEntity : public EditorCommandBase {
public:
    EditorCommandAddEntity(EntityType p_entity_type)
        : EditorCommandBase(COMMAND_TYPE_ENTITY_CREATE), m_entityType(p_entity_type) {}

    virtual void Execute(Scene& p_scene) override;

protected:
    EntityType m_entityType;
    ecs::Entity m_parent;
    ecs::Entity m_entity;

    friend class EditorLayer;
};

class EditorCommandAddComponent : public EditorCommandBase {
public:
    EditorCommandAddComponent(ComponentType p_component_type)
        : EditorCommandBase(COMMAND_TYPE_COMPONENT_ADD), m_componentType(p_component_type) {}

    virtual void Execute(Scene& p_scene) override;

protected:
    ComponentType m_componentType;
    ecs::Entity target;

    friend class EditorLayer;
};

class EditorCommandRemoveEntity : public EditorCommandBase {
public:
    EditorCommandRemoveEntity(ecs::Entity p_target)
        : EditorCommandBase(COMMAND_TYPE_ENTITY_REMOVE), m_target(p_target) {}

    virtual void Execute(Scene& p_scene) override;

protected:
    ecs::Entity m_target;

    friend class EditorLayer;
};

class OpenProjectCommand : public EditorCommandBase {
public:
    OpenProjectCommand(bool p_open_dialog) : EditorCommandBase(COMMAND_TYPE_OPEN_PROJECT), m_openDialog(p_open_dialog) {}

    virtual void Execute(Scene& p_scene) override;

protected:
    bool m_openDialog;
};

class SaveProjectCommand : public EditorCommandBase {
public:
    SaveProjectCommand(bool p_open_dialog) : EditorCommandBase(COMMAND_TYPE_SAVE_PROJECT), m_openDialog(p_open_dialog) {}

    virtual void Execute(Scene& p_scene) override;

protected:
    bool m_openDialog;
};

class UndoViewerCommand : public EditorCommandBase {
public:
    UndoViewerCommand() : EditorCommandBase(COMMAND_TYPE_UNDO_VIEWER) {}

    virtual void Execute(Scene& p_scene) override;
};

class RedoViewerCommand : public EditorCommandBase {
public:
    RedoViewerCommand() : EditorCommandBase(COMMAND_TYPE_REDO_VIEWER) {}

    virtual void Execute(Scene& p_scene) override;
};

class EntityTransformCommand : public EditorUndoCommandBase {
public:
    EntityTransformCommand(CommandType p_type,
                           Scene& p_scene,
                           ecs::Entity p_entity,
                           const Matrix4x4f& p_before,
                           const Matrix4x4f& p_after);

    void Undo() override;
    void Redo() override;

    bool MergeCommand(const UndoCommand* p_command) override;

protected:
    Scene& m_scene;
    ecs::Entity m_entity;

    Matrix4x4f m_before;
    Matrix4x4f m_after;
};

}  // namespace my
