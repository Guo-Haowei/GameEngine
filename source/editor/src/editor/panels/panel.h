#pragma once
#include "imgui/imgui.h"
#include "scene/scene.h"

namespace my {

class EditorLayer;

class Panel {
public:
    Panel(const std::string& name, EditorLayer& editor) : m_name(name), m_editor(editor) {}

    void update(my::Scene&);

protected:
    virtual void update_internal(my::Scene&) {}

    std::string m_name;
    EditorLayer& m_editor;
};

}  // namespace my
