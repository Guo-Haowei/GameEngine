#pragma once
#include "editor/editor_item.h"
#include "scene/scene.h"

namespace my {

class MenuBar : public EditorItem {
public:
    MenuBar(EditorLayer& p_editor) : EditorItem(p_editor) {}

    void update(Scene& p_scene) override;
};

}  // namespace my
