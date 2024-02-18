#pragma once

namespace my {

class EditorLayer;

class EditorItem {
public:
    EditorItem(EditorLayer& editor) : m_editor(editor) {}
    virtual ~EditorItem() = default;

protected:
    EditorLayer& m_editor;
};

}  // namespace my
