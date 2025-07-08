#include "tilemap_panel.h"

#include "editor/editor_layer.h"
#include "engine/runtime/asset_registry.h"

namespace my {

TileMapPanel::TileMapPanel(EditorLayer& p_editor) : EditorWindow("Tile Map", p_editor) {
}

void TileMapPanel::OnAttach() {
    auto asset_registry = m_editor.GetApplication()->GetAssetRegistry();
    m_tileset = asset_registry->GetAssetByHandle<ImageAsset>(AssetHandle{ "@res://images/tiles.png" });
}

void TileMapPanel::TilePaint() {
    const auto texture = m_tileset->gpu_texture;
    if (!texture) {
        return;
    }

    const auto handle = texture->GetHandle();
    ImVec2 imageSize((float)texture->desc.width, (float)texture->desc.height);

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::Image(handle, imageSize);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    int gridX = 3;
    int gridY = 2;

    // Input
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isHovered = ImGui::IsItemHovered();
    bool isClicked = ImGui::IsItemClicked();

    static int selectedX = -1, selectedY = -1;

    if (isHovered && isClicked) {
        float cellWidth = imageSize.x / gridX;
        float cellHeight = imageSize.y / gridY;

        int localX = (int)((mousePos.x - cursor.x) / cellWidth);
        int localY = (int)((mousePos.y - cursor.y) / cellHeight);

        // Clamp to valid range
        if (localX >= 0 && localX < gridX && localY >= 0 && localY < gridY) {
            selectedX = localX;
            selectedY = localY;
        }
    }

    for (int i = 1; i < gridY; ++i) {
        float y = cursor.y + i * (imageSize.y / gridY);
        drawList->AddLine(ImVec2(cursor.x, y),
                          ImVec2(cursor.x + imageSize.x, y),
                          IM_COL32(255, 255, 255, 255));
    }

    for (int j = 1; j < gridX; ++j) {
        float x = cursor.x + j * (imageSize.x / gridX);
        drawList->AddLine(ImVec2(x, cursor.y),
                          ImVec2(x, cursor.y + imageSize.y),
                          IM_COL32(255, 255, 255, 255));
    }

    if (selectedX >= 0 && selectedY >= 0) {
        float cellW = imageSize.x / gridX;
        float cellH = imageSize.y / gridY;

        ImVec2 pMin = ImVec2(cursor.x + selectedX * cellW, cursor.y + selectedY * cellH);
        ImVec2 pMax = ImVec2(pMin.x + cellW, pMin.y + cellH);

        drawList->AddRectFilled(pMin, pMax, IM_COL32(0, 255, 0, 100));  // green transparent overlay
        drawList->AddRect(pMin, pMax, IM_COL32(0, 255, 0, 255));        // solid border
    }
}

void TileMapPanel::TileSetup() {
    // Tabs: Setup | Select | Paint
    if (ImGui::BeginTabBar("TileSetModes")) {
        if (ImGui::BeginTabItem("Setup")) {
            // Margin/Separation etc.
            static int margin_x = 0, margin_y = 0;
            static int sep_x = 0, sep_y = 0;

            ImGui::Text("Texture:");
            // ImGui::Image(myTexID, ImVec2(64, 64));
            ImGui::InputInt("Margin X", &margin_x);
            ImGui::InputInt("Margin Y", &margin_y);
            ImGui::InputInt("Separation X", &sep_x);
            ImGui::InputInt("Separation Y", &sep_y);

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

    ImGui::EndChild();
}

}  // namespace my
