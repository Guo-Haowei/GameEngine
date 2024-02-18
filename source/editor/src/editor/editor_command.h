#pragma once
#include "scene/scene.h"

namespace my {

enum EditorCommandName : uint32_t {
    EDITOR_COMMAND_ADD_LIGHT_OMNI,
    EDITOR_COMMAND_ADD_LIGHT_POINT,
    EDITOR_COMMAND_ADD_MESH_PLANE,
    EDITOR_COMMAND_ADD_MESH_CUBE,
    EDITOR_COMMAND_ADD_MESH_SPHERE,
    EDITOR_COMMAND_MAX,
};

struct EditorCommand {
    EditorCommand(EditorCommandName p_name) : name(p_name) {}
    virtual ~EditorCommand() = default;
    EditorCommandName name;
};

struct EditorCommandAdd : public EditorCommand {
    EditorCommandAdd(EditorCommandName name) : EditorCommand(name) {}

    ecs::Entity parent;
    ecs::Entity entity;
};

}  // namespace my
