#include "asset_inspector.h"

#include "engine/assets/assets.h"
#include "editor/editor_layer.h"
#include "editor/widget.h"
#include "engine/runtime/asset_registry.h"

namespace my {

AssetInspector::AssetInspector(EditorLayer& p_editor)
    : EditorWindow("Asset Inspector", p_editor) {
}

void AssetInspector::OnAttach() {
    m_asset_registry = m_editor.GetApplication()->GetAssetRegistry();
}

void AssetInspector::TilePaint(SpriteSheetAsset& p_sprite) {
    ImGui::Text("TileSet");

    auto& handle = p_sprite.image_handle;
    if (!handle.IsReady()) {
        return;
    }

    const ImageAsset& image = static_cast<ImageAsset&>(*(handle.entry->asset.get()));

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 tile_size((float)image.width, (float)image.height);

    ImGui::InvisibleButton("TileClickable", tile_size);  // enables interaction
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();

    // Draw tileset
    {
        draw_list->AddImage(
            image.gpu_texture->GetHandle(),
            cursor,
            cursor + tile_size,
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 255));
    }

    const int sep_x = p_sprite.separation.x;
    const int sep_y = p_sprite.separation.y;

    if (sep_x <= 0 || sep_y <= 0) {
        return;
    }

    const int num_col = static_cast<int>(tile_size.x / sep_x);
    const int num_row = static_cast<int>(tile_size.y / sep_y);

    if (hovered && clicked) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float cellWidth = tile_size.x / num_col;
        float cellHeight = tile_size.y / num_row;

        int local_x = (int)((mouse_pos.x - cursor.x) / cellWidth);
        int local_y = (int)((mouse_pos.y - cursor.y) / cellHeight);

        // Clamp to valid range
        if (local_x >= 0 && local_x < num_col && local_y >= 0 && local_y < num_row) {
            m_selected_x = local_x;
            m_selected_y = local_y;
        }
    }

    for (int dx = sep_x; dx < tile_size.x; dx += sep_x) {
        draw_list->AddLine(ImVec2(cursor.x + dx, cursor.y),
                           ImVec2(cursor.x + dx, cursor.y + tile_size.y),
                           IM_COL32(255, 255, 255, 255));
    }

    for (int dy = sep_y; dy < tile_size.y; dy += sep_y) {
        draw_list->AddLine(ImVec2(cursor.x, cursor.y + dy),
                           ImVec2(cursor.x + tile_size.x, cursor.y + dy),
                           IM_COL32(255, 255, 255, 255));
    }

    if (m_selected_x >= 0 && m_selected_y >= 0) {
        float cellW = tile_size.x / num_col;
        float cellH = tile_size.y / num_row;

        ImVec2 pMin = ImVec2(cursor.x + m_selected_x * cellW, cursor.y + m_selected_y * cellH);
        ImVec2 pMax = ImVec2(pMin.x + cellW, pMin.y + cellH);

        draw_list->AddRectFilled(pMin, pMax, IM_COL32(0, 255, 0, 100));  // green transparent overlay
        draw_list->AddRect(pMin, pMax, IM_COL32(0, 255, 0, 255));        // solid border

        m_editor.context.selected_tile = m_selected_x + m_selected_y * 3 + 1;
    } else {
        m_editor.context.selected_tile = -1;
    }
}

void AssetInspector::TileSetup(SpriteSheetAsset& p_sprite) {
    if (ImGui::BeginTabBar("TileSetModes")) {
        if (ImGui::BeginTabItem("Setup")) {
            if (p_sprite.image_handle.IsReady()) {
                ImGui::Text("Image: %s", p_sprite.image_handle.entry->metadata.path.c_str());
            }

            ImGui::InputInt("Separation X", &p_sprite.separation.x);
            ImGui::InputInt("Separation Y", &p_sprite.separation.y);

            // ImGui::Checkbox("Use Texture Region", &use_region);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Select")) {
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void AssetInspector::DropRegion(SpriteSheetAsset& p_sprite) {
    ImGui::Text("Image");

    const float w = 300;
    if (p_sprite.image_handle.IsReady()) {
        const auto& asset = static_cast<ImageAsset&>(*p_sprite.image_handle.entry->asset.get());
        const float h = w / asset.width * asset.height;
        ImVec2 size = ImVec2(w, h);

        ImGui::Image(asset.gpu_texture->GetHandle(), size);
    } else {
        ImVec2 size = ImVec2(w, w);

        ImGui::InvisibleButton("DropTarget", size);
    }

    // @TODO: refactor
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_PAYLOAD_TYPE")) {
            std::string& texture = *((std::string*)payload->Data);
            LOG_OK("{}", texture);

            auto image_handle = m_asset_registry->Request(texture);
            if (image_handle.IsReady() && image_handle.entry->asset->type == AssetType::Image) {
                p_sprite.image_id = image_handle.guid;
                p_sprite.image_handle = image_handle;
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void AssetInspector::DrawSprite(SpriteSheetAsset& p_sprite) {
    float full_width = ImGui::GetContentRegionAvail().x;
    constexpr float sprite_source_tab_width = 360.0f;  // left panel fixed width
    constexpr float sprite_data_tab = 360.0f;
    const float main_width = full_width - sprite_source_tab_width - sprite_data_tab - ImGui::GetStyle().ItemSpacing.x;

    ImGui::BeginChild("ImageSource", ImVec2(sprite_source_tab_width, 0), true);
    DropRegion(p_sprite);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("SpriteEditor", ImVec2(sprite_data_tab, 0), true);
    TileSetup(p_sprite);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("TileSetPanel", ImVec2(main_width, 0), true);
    TilePaint(p_sprite);
    ImGui::EndChild();
}

void AssetInspector::UpdateInternal(Scene&) {
    IAsset* asset = m_editor.context.selected_asset.get();
    if (!asset) {
        return;
    }

    switch (asset->type.GetData()) {
        case AssetType::SpriteSheet:
            DrawSprite(static_cast<SpriteSheetAsset&>(*asset));
            break;
        default:
            break;
    }
}

}  // namespace my
