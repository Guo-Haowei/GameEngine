#pragma once
#include "scene/scene.h"

namespace my {

enum EditorCommandType : uint8_t {
    EDITOR_ADD_ENTITY,
    EDITOR_REMOVE_ENTITY,
    EDITOR_ADD_COMPONENT,
};

enum EntityType : uint8_t {
    ENTITY_TYPE_INFINITE_LIGHT,
    ENTITY_TYPE_POINT_LIGHT,
    ENTITY_TYPE_AREA_LIGHT,
    ENTITY_TYPE_PLANE,
    ENTITY_TYPE_CUBE,
    ENTITY_TYPE_SPHERE,
};

enum ComponentType : uint8_t {
    COMPONENT_TYPE_BOX_COLLIDER,
    COMPONENT_TYPE_MESH_COLLIDER,
    COMPONENT_TYPE_SPHERE_COLLIDER,
};

class EditorCommand {
public:
    EditorCommand(EditorCommandType p_type) : type(p_type) {}
    virtual ~EditorCommand() = default;

protected:
    EditorCommandType type;
};

// @TODO: make proctected
class EditorCommandAddEntity : public EditorCommand {
public:
    EditorCommandAddEntity(EntityType p_entity_type)
        : EditorCommand(EDITOR_ADD_ENTITY), entityType(p_entity_type) {}

    EntityType entityType;
    ecs::Entity parent;
    ecs::Entity entity;
};

class EditorCommandAddComponent : public EditorCommand {
public:
    EditorCommandAddComponent(ComponentType p_component_type)
        : EditorCommand(EDITOR_ADD_COMPONENT), componentType(p_component_type) {}

    ComponentType componentType;
    ecs::Entity target;
};

class EditorCommandRemoveEntity : public EditorCommand {
public:
    EditorCommandRemoveEntity(ecs::Entity p_target)
        : EditorCommand(EDITOR_REMOVE_ENTITY), target(p_target) {}

    ecs::Entity target;
};

}  // namespace my
