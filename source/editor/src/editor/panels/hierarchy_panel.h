#pragma once
#include "panel.h"

namespace my {

class HierarchyPanel : public Panel {
public:
    HierarchyPanel(EditorLayer& editor) : Panel("Scene", editor) {}

protected:
    void update_internal(Scene& scene) override;

private:
    void draw_popup(Scene& scene);
};

}  // namespace my
