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

class EditorCompositeWindow : public EditorItem {
public:
    EditorCompositeWindow(EditorLayer& p_editor) : EditorItem(p_editor) {}

    void Update(Scene&) override;

protected:
    void AddWindow(std::shared_ptr<EditorWindow> p_window);

    std::vector<std::shared_ptr<EditorWindow>> m_windows;
};

}  // namespace my
