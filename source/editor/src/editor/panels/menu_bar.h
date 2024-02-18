#pragma once
#include "editor/editor_item.h"
#include "scene/scene.h"

namespace my {

class MenuBar : public EditorItem {
public:
    MenuBar(EditorLayer& editor) : EditorItem(editor) {}

    void draw(Scene& scene);
};

}  // namespace my
