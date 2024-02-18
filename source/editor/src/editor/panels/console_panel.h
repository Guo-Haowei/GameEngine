#pragma once
#include "editor/editor_window.h"

namespace my {

class ConsolePanel : public EditorWindow {
public:
    ConsolePanel(EditorLayer& editor) : EditorWindow("Console", editor) {}

protected:
    void update_internal(Scene& scene) override;

private:
    bool m_auto_scroll = true;
    bool m_scroll_to_bottom = false;
};

}  // namespace my
