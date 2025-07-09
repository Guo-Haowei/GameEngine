#include "tilemap_panel.h"

#include "editor/editor_layer.h"
#include "editor/widget.h"
#include "engine/runtime/asset_registry.h"

namespace my {

TileMapPanel::TileMapPanel(EditorLayer& p_editor) : EditorWindow("Tile Map", p_editor) {
}

void TileMapPanel::OnAttach() {
    auto asset_registry = m_editor.GetApplication()->GetAssetRegistry();
    m_tileset = asset_registry->GetAssetByHandle<ImageAsset>(AssetHandle{ "@res://images/tiles.png" });
    m_checkerboard = asset_registry->GetAssetByHandle<ImageAsset>(AssetHandle{ "@res://images/checkerboard.jpg" });
}

void TileMapPanel::TilePaint() {
    const auto tileset = m_tileset->gpu_texture;
    const auto checker = m_checkerboard->gpu_texture;
    if (!tileset || !checker) {
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 tile_size((float)tileset->desc.width, (float)tileset->desc.height);

    ImGui::InvisibleButton("tile_clickable", tile_size);  // enables interaction
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();

    // Draw checker board
    {
        // @TODO: adjust size based on tile size
        constexpr float CHECKER_SIZE = 2048;
        ImVec2 desired_size = tile_size;
        const float ratio = (2.0f / CHECKER_SIZE);
        ImVec2 uv = desired_size;
        uv.x *= ratio;
        uv.y *= ratio;

        draw_list->AddImage(
            checker->GetHandle(),
            cursor,
            cursor + desired_size,
            ImVec2(0, 0), uv,
            IM_COL32(255, 255, 255, 255)
        );
    }

    // Draw tileset
    {
        draw_list->AddImage(
            tileset->GetHandle(),
            cursor,
            cursor + tile_size,
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 255));
    }

    if (m_sep.x <= 0 || m_sep.y <= 0) {
        return;
    }

    const int num_col = static_cast<int>(tile_size.x / m_sep.x);
    const int num_row = static_cast<int>(tile_size.y / m_sep.y);

    static int selectedX = -1, selectedY = -1;
    if (hovered && clicked) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        float cellWidth = tile_size.x / num_col;
        float cellHeight = tile_size.y / num_row;

        int localX = (int)((mouse_pos.x - cursor.x) / cellWidth);
        int localY = (int)((mouse_pos.y - cursor.y) / cellHeight);

        // Clamp to valid range
        if (localX >= 0 && localX < num_col && localY >= 0 && localY < num_row) {
            selectedX = localX;
            selectedY = localY;
        }
    }

    for (int dx = m_sep.x; dx < tile_size.x; dx += m_sep.x) {
        draw_list->AddLine(ImVec2(cursor.x + dx, cursor.y),
                          ImVec2(cursor.x + dx, cursor.y + tile_size.y),
                          IM_COL32(255, 255, 255, 255));
    }

    for (int dy = m_sep.y; dy < tile_size.y; dy += m_sep.y) {
        draw_list->AddLine(ImVec2(cursor.x, cursor.y + dy),
                          ImVec2(cursor.x + tile_size.x, cursor.y + dy),
                          IM_COL32(255, 255, 255, 255));
    }

    if (selectedX >= 0 && selectedY >= 0) {
        float cellW = tile_size.x / num_col;
        float cellH = tile_size.y / num_row;

        ImVec2 pMin = ImVec2(cursor.x + selectedX * cellW, cursor.y + selectedY * cellH);
        ImVec2 pMax = ImVec2(pMin.x + cellW, pMin.y + cellH);

        draw_list->AddRectFilled(pMin, pMax, IM_COL32(0, 255, 0, 100));  // green transparent overlay
        draw_list->AddRect(pMin, pMax, IM_COL32(0, 255, 0, 255));        // solid border
    }
}

void TileMapPanel::TileSetup() {
    // Tabs: Setup | Select | Paint
    if (ImGui::BeginTabBar("TileSetModes")) {
        if (ImGui::BeginTabItem("Setup")) {

            ImGui::Text("Texture:");
            // ImGui::Image(myTexID, ImVec2(64, 64));
            ImGui::InputInt("Separation X", &m_sep.x);
            ImGui::InputInt("Separation Y", &m_sep.y);

            // ImGui::Checkbox("Use Texture Region", &use_region);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Select")) {
            // Draw selection panel (as shown earlier)
            // selector.Draw();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Paint")) {
            ImGui::Text("Paint mode");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void TileMapPanel::UpdateInternal(Scene&) {

    float fullWidth = ImGui::GetContentRegionAvail().x;
    float sidebarWidth = 300.0f;  // left panel fixed width
    float mainWidth = fullWidth - sidebarWidth - ImGui::GetStyle().ItemSpacing.x;

    ImGui::BeginChild("TileSources", ImVec2(sidebarWidth, 0), true);
    ImGui::Text("Tile Sources");

    TileSetup();

    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("TileSetPanel", ImVec2(mainWidth, 0), true);
    ImGui::Text("TileSet");

    TilePaint();

    {
        static bool toggle = false;
        ToggleButton("my toggle", &toggle);
    }

    ImGui::EndChild();
}

}  // namespace my
