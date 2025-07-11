#include "scene_component_2d.h"

#include "engine/renderer/gpu_resource.h"
#include "engine/runtime/graphics_manager_interface.h"

namespace my {

void TileMapComponent::FromArray(const std::vector<std::vector<int>>& p_data) {
    m_width = static_cast<int>(p_data[0].size());
    m_height = static_cast<int>(p_data.size());

    m_tiles.resize(m_width * m_height);
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            m_tiles[y * m_width + x] = p_data[m_height - 1 - y][x];
            // m_tiles[y * m_width + x] = p_data[y][x];
        }
    }

    SetDirty();
}

void TileMapComponent::SetTile(int p_x, int p_y, int p_tile_id) {
    if (p_x >= 0 && p_x < m_width && p_y >= 0 && p_y < m_height) {
        int& old = m_tiles[p_y * m_width + p_x];
        if (old != p_tile_id) {
            old = p_tile_id;
            SetDirty();
        }
    }
}

int TileMapComponent::GetTile(int p_x, int p_y) const {
    if (p_x >= 0 && p_x < m_width && p_y >= 0 && p_y < m_height) {
        return m_tiles[p_y * m_width + p_x];
    }
    return 0;
}

void TileMapComponent::SetDimensions(int p_width, int p_height) {
    m_width = p_width;
    m_height = p_height;
    m_tiles.resize(p_width * p_height);
}

void TileMapComponent::CreateRenderData() {
    const int w = m_width;
    const int h = m_height;
    DEV_ASSERT(w && h);

    std::vector<Vector2f> vertices;
    std::vector<Vector2f> uvs;
    std::vector<uint32_t> indices;
    vertices.reserve(w * h * 4);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int tile = m_tiles[y * m_width + x];
            if (tile == 0) {
                continue;
            }

            const float s = 1.0f;
            float x0 = s * x;
            float y0 = s * y;
            float x1 = s * (x + 1);
            float y1 = s * (y + 1);
            Vector2f bottom_left{ x0, y0 };
            Vector2f bottom_right{ x1, y0 };
            Vector2f top_left{ x0, y1 };
            Vector2f top_right{ x1, y1 };
            Vector2f uv_min = m_sprite.frames[tile - 1].GetMin();
            Vector2f uv_max = m_sprite.frames[tile - 1].GetMax();
            Vector2f uv0 = uv_min;
            Vector2f uv1 = { uv_max.x, uv_min.y };
            Vector2f uv2 = { uv_min.x, uv_max.y };
            Vector2f uv3 = uv_max;

            const uint32_t offset = (uint32_t)vertices.size();
            vertices.push_back(bottom_left);
            vertices.push_back(bottom_right);
            vertices.push_back(top_left);
            vertices.push_back(top_right);

            uvs.push_back(uv0);
            uvs.push_back(uv1);
            uvs.push_back(uv2);
            uvs.push_back(uv3);

            indices.push_back(0 + offset);
            indices.push_back(1 + offset);
            indices.push_back(3 + offset);

            indices.push_back(0 + offset);
            indices.push_back(3 + offset);
            indices.push_back(2 + offset);
        }
    }
    uint32_t count = (uint32_t)indices.size();

    GpuBufferDesc buffers[2];
    GpuBufferDesc buffer_desc;
    buffer_desc.type = GpuBufferType::VERTEX;
    buffer_desc.elementSize = sizeof(Vector2f);
    buffer_desc.elementCount = (uint32_t)vertices.size();
    buffer_desc.initialData = vertices.data();

    buffers[0] = buffer_desc;

    buffer_desc.initialData = uvs.data();
    buffers[1] = buffer_desc;

    GpuBufferDesc index_desc;
    index_desc.type = GpuBufferType::INDEX;
    index_desc.elementSize = sizeof(uint32_t);
    index_desc.elementCount = count;
    index_desc.initialData = indices.data();

    GpuMeshDesc desc;
    desc.drawCount = count;
    desc.enabledVertexCount = 2;
    desc.vertexLayout[0] = GpuMeshDesc::VertexLayout{ 0, sizeof(Vector2f), 0 };
    desc.vertexLayout[1] = GpuMeshDesc::VertexLayout{ 1, sizeof(Vector2f), 0 };

    // @TODO: refactor this part
    auto mesh = IGraphicsManager::GetSingleton().CreateMeshImpl(desc, 2, buffers, &index_desc);

    m_mesh = *mesh;
}

}  // namespace my
