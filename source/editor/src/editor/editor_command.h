#pragma once
#include "scene/scene.h"

namespace my {

enum class EditorCommandType : uint8_t {
    ADD_ENTITY,
    REMOVE_ENTITY,
    ADD_COMPONENT,
};

enum class EntityType : uint8_t {
    INFINITE_LIGHT,
    POINT_LIGHT,
    AREA_LIGHT,
    PLANE,
    CUBE,
    SPHERE,
    CYLINDER,
    TORUS,
    TRANSFORM,
    PARTICLE_EMITTER,
    FORCE_FIELD,
};

enum class ComponentType : uint8_t {
    BOX_COLLIDER,
    MESH_COLLIDER,
    SPHERE_COLLIDER,
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
        : EditorCommand(EditorCommandType::ADD_ENTITY), entityType(p_entity_type) {}

    EntityType entityType;
    ecs::Entity parent;
    ecs::Entity entity;
};

class EditorCommandAddComponent : public EditorCommand {
public:
    EditorCommandAddComponent(ComponentType p_component_type)
        : EditorCommand(EditorCommandType::ADD_COMPONENT), componentType(p_component_type) {}

    ComponentType componentType;
    ecs::Entity target;
};

class EditorCommandRemoveEntity : public EditorCommand {
public:
    EditorCommandRemoveEntity(ecs::Entity p_target)
        : EditorCommand(EditorCommandType::REMOVE_ENTITY), target(p_target) {}

    ecs::Entity target;
};

}  // namespace my
