#pragma once
#include "editor/editor_window.h"

namespace my {

struct ImageAsset;

// @TODO: name it to content window or something
class TileMapPanel : public EditorWindow {
public:
    TileMapPanel(EditorLayer& p_editor);

    void OnAttach() override;

protected:
    void UpdateInternal(Scene&) override;

    void TileSetup();
    void TilePaint();

    // @TODO: refactor
    const ImageAsset* m_tileset{ nullptr };
    const ImageAsset* m_checkerboard{ nullptr };

    Vector2i m_sep{ 64, 64 };

    int m_selected_x = -1;
    int m_selected_y = -1;
};

}  // namespace my
