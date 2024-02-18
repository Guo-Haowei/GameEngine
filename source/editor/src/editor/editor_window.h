#pragma once
#include "editor/editor_item.h"

namespace my {

class EditorWindow : public EditorItem {
public:
    EditorWindow(const std::string& name, EditorLayer& editor) : EditorItem(editor), m_name(name) {}

    void update(Scene&);

protected:
    virtual void update_internal(Scene&) = 0;

    std::string m_name;
};

}  // namespace my
