#pragma once
#include "panel.h"

namespace my {

class MenuBar : public Panel {
public:
    MenuBar(EditorLayer& editor) : Panel("MenuBar", editor) {}

    // @TODO: fix this ugly shit
    void update(Scene& scene) override;

protected:
    void update_internal(Scene&) override {}
};

}  // namespace my
