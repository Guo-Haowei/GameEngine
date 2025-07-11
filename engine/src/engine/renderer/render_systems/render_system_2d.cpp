#include "engine/renderer/frame_data.h"
#include "engine/scene/scene.h"

namespace my {

void RunTileMapRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    auto view = p_scene.View<TileMapComponent>();
    for (const auto [id, tileMap] : view) {
        const TransformComponent& transform = *p_scene.GetComponent<TransformComponent>(id);

        // Should move tile map logic to somewhere else
        if (tileMap.IsDirty()) {
            tileMap.CreateRenderData();
            tileMap.SetDirty(false);
        }

        const Matrix4x4f& world_matrix = transform.GetWorldMatrix();
        PerBatchConstantBuffer batch_buffer;
        batch_buffer.c_worldMatrix = world_matrix;

        DrawCommand draw;
        draw.indexCount = tileMap.m_mesh->desc.drawCount;
        draw.mesh_data = tileMap.m_mesh.get();
        draw.batch_idx = p_framedata.batchCache.FindOrAdd(id, batch_buffer);

        // @TODO: make a render command
        p_framedata.tile_maps.push_back(RenderCommand::From(draw));
    }
}

}  // namespace my
