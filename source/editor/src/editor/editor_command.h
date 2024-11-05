#pragma once
#include "scene/scene.h"

namespace my {

enum class EditorCommandType : uint8_t {
    ADD_ENTITY,
    REMOVE_ENTITY,
    ADD_COMPONENT,
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
