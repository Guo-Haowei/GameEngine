#include "engine/renderer/frame_data.h"
#include "engine/scene/scene.h"

namespace my {

void RunTileMapRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    unused(p_framedata);

    auto view = p_scene.View<TileMapComponent>();
    for (const auto [id, tileMap] : view) {
        const int w = tileMap.m_width;
        const int h = tileMap.m_height;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int tile = tileMap.m_tiles[y * w + x];
                unused(tile);
            }
        }
    }
}

}  // namespace my
