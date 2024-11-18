#pragma once
#include "editor/editor_window.h"

namespace my {

class LogPanel : public EditorWindow {
public:
    LogPanel(EditorLayer& editor) : EditorWindow("Log", editor) {}

protected:
    void UpdateInternal(Scene& scene) override;

private:
    bool m_autoScroll{ true };
    bool m_scrollToBottom{ false };
    LogLevel m_filter{ LOG_LEVEL_ALL };
};

}  // namespace my
