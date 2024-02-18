#pragma once
#include "scene/scene.h"

namespace my {

class EditorLayer;

#define POPUP_NAME_ID "SCENE_PANEL_POPUP"

class EditorItem {
public:
    EditorItem(EditorLayer& editor) : m_editor(editor) {}
    virtual ~EditorItem() = default;

protected:
    void open_add_entity_popup(ecs::Entity parent);

    EditorLayer& m_editor;
};

}  // namespace my
