#pragma once
#include "engine/assets/asset_interface.h"
#include "editor/editor_window.h"

namespace my {

class AssetRegistry;

class AssetInspector : public EditorWindow {
public:
    AssetInspector(EditorLayer& p_editor);

    void OnAttach() override;

protected:
    void UpdateInternal(Scene&) override;

    void DropRegion(SpriteSheetAsset& p_sprite);
    void DrawSprite(SpriteSheetAsset& p_sprite);

    void TileSetup(SpriteSheetAsset& p_sprite);
    void TilePaint(SpriteSheetAsset& p_sprite);

    // @TODO: refactor

    int m_selected_x = -1;
    int m_selected_y = -1;
    // @TODO: refactor

    AssetRegistry* m_asset_registry;
};

}  // namespace my
