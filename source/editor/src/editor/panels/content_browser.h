#pragma once
#include "editor/editor_window.h"

namespace my {

class ContentBrowser : public EditorCompositeWindow {
public:
    ContentBrowser(EditorLayer& p_editor);

    ecs::Entity get_selected() const { return m_selected; }
    void set_selected(ecs::Entity p_entity);

private:
    ecs::Entity m_selected;
};

}  // namespace my
