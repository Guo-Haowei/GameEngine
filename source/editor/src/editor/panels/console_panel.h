#pragma once
#include "panel.h"

namespace my {

class ConsolePanel : public Panel {
public:
    ConsolePanel(EditorLayer& editor) : Panel("Console", editor) {}

protected:
    void update_internal(Scene& scene) override;

private:
    bool m_auto_scroll = true;
    bool m_scroll_to_bottom = false;
};

}  // namespace my
