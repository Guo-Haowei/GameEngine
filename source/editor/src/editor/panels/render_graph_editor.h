#pragma once
#include "editor/editor_window.h"
#include "editor/utility/graph_editor.h"

namespace my {

class RenderGraphEditorDelegate;

class RenderGraphEditor : public EditorWindow {
public:
    RenderGraphEditor(EditorLayer& editor);

protected:
    void update_internal(Scene& scene) override;

private:
    GraphEditor::Options m_options;
    GraphEditor::ViewState m_view_state;
    GraphEditor::FitOnScreen m_fit = GraphEditor::Fit_None;
    std::shared_ptr<RenderGraphEditorDelegate> m_delegate;
};

}  // namespace my
