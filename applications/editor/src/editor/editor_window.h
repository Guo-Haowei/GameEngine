#pragma once
#include "editor/editor_item.h"

namespace my {

class EditorWindow : public EditorItem {
public:
    EditorWindow(const std::string& p_name, EditorLayer& p_editor) : EditorItem(p_editor), m_name(p_name) {}

    void Update(Scene&) override;

protected:
    virtual void UpdateInternal(Scene&) = 0;

    std::string m_name;
};

}  // namespace my
