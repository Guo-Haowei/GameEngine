#include "engine/renderer/frame_data.h"
#include "engine/assets/asset.h"
#include "engine/scene/scene.h"

namespace my {

void RunTileMapRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    auto view = p_scene.View<TileMapComponent>();
    for (const auto [id, tileMap] : view) {

        // Should move tile map logic to somewhere else
        // But this is a editor only logic, we are not going to update it in actual game
        if (tileMap.IsDirty()) {
            tileMap.CreateRenderData();
            tileMap.SetDirty(false);
        }

        if (tileMap.m_sprite.texture->gpu_texture) {
            const TransformComponent& transform = *p_scene.GetComponent<TransformComponent>(id);

            const Matrix4x4f& world_matrix = transform.GetWorldMatrix();
            PerBatchConstantBuffer batch_buffer;
            batch_buffer.c_worldMatrix = world_matrix;

            DrawCommand draw;
            draw.indexCount = tileMap.m_mesh->desc.drawCount;
            draw.mesh_data = tileMap.m_mesh.get();
            draw.batch_idx = p_framedata.batchCache.FindOrAdd(id, batch_buffer);

            // @TODO: ?
            draw.texture = tileMap.m_sprite.texture->gpu_texture.get();

            // @TODO: make a render command
            p_framedata.tile_maps.push_back(RenderCommand::From(draw));
        }
    }
}

}  // namespace my
