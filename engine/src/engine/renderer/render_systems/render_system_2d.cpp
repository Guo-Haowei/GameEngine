#include "engine/renderer/frame_data.h"
#include "engine/scene/scene.h"

namespace my {

void RunTileMapRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    auto view = p_scene.View<TileMapComponent>();
    for (const auto [id, tileMap] : view) {
        // quick and dirty

        if (tileMap.IsDirty()) {
            tileMap.CreateRenderData();
            tileMap.SetDirty(false);
        }

        p_framedata.tiles = tileMap.m_mesh;
    }
}

}  // namespace my
