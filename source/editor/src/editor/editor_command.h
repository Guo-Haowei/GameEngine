#pragma once
#include "scene/scene.h"
#include "shared/undo_command.h"

namespace my {

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

// @TODO: make proctected
class EditorCommandAddEntity : public ICommand {
public:
    EditorCommandAddEntity(EntityType p_entity_type)
        : ICommand(CommandType::ADD_ENTITY), entityType(p_entity_type) {}

    EntityType entityType;
    ecs::Entity parent;
    ecs::Entity entity;
};

class EditorCommandAddComponent : public ICommand {
public:
    EditorCommandAddComponent(ComponentType p_component_type)
        : ICommand(CommandType::ADD_COMPONENT), componentType(p_component_type) {}

    ComponentType componentType;
    ecs::Entity target;
};

class EditorCommandRemoveEntity : public ICommand {
public:
    EditorCommandRemoveEntity(ecs::Entity p_target)
        : ICommand(CommandType::REMOVE_ENTITY), target(p_target) {}

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

}  // namespace my
