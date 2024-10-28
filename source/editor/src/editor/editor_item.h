#pragma once
#include "scene/scene.h"

namespace my {

class EditorLayer;

class EditorItem {
public:
    inline static constexpr const char* DRAG_DROP_ENV = "DRAG_DROP_ENV";
    inline static constexpr const char* DRAG_DROP_IMPORT = "DRAG_DROP_IMPORT";

    EditorItem(EditorLayer& p_editor) : m_editor(p_editor) {}
    virtual ~EditorItem() = default;

    virtual void Update(Scene&) = 0;

protected:
    void OpenAddEntityPopup(ecs::Entity p_parent);

    EditorLayer& m_editor;
};

}  // namespace my
