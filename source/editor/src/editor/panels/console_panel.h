#pragma once
#include "editor/editor_window.h"

namespace my {

class ConsolePanel : public EditorWindow {
public:
    ConsolePanel(EditorLayer& editor) : EditorWindow("Console", editor) {}

protected:
    void UpdateInternal(Scene& scene) override;

private:
    bool m_autoScroll = true;
    bool m_scrollToBottom = false;
};

}  // namespace my
