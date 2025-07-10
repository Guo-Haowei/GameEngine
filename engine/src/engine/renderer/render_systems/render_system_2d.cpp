#include "engine/renderer/frame_data.h"
#include "engine/scene/scene.h"

namespace my {

void RunTileMapRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    auto view = p_scene.View<TileMapComponent>();
    for (const auto [id, tileMap] : view) {
        // quick and dirty
        p_framedata.tiles.reset(new TileMapComponent);
        *p_framedata.tiles = tileMap;
    }
}

}  // namespace my
