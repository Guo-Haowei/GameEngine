#pragma once
#include "engine/math/box.h"
#include "engine/math/geomath.h"
#include "scene_component_base.h"

namespace my {

class Archive;
struct GpuMesh;
struct ImageAsset;

struct SpriteSheet {
    std::vector<Rect> frames;

    // not serialized
    mutable const ImageAsset* texture;

    const Rect& getFrame(int index) const { return frames[index]; }
};

class TileMapComponent : public ComponentFlagBase {
public:
    void FromArray(const std::vector<std::vector<int>>& p_data);

    void SetTile(int p_x, int p_y, int p_tile_id);

    int GetTile(int p_x, int p_y) const;

    void SetDimensions(int p_width, int p_height);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    void CreateRenderData();

    void Serialize(Archive& p_archive, uint32_t p_version);

    void OnDeserialized() {}

    static void RegisterClass();

    // @TODO: change to private
public:
    int m_width{ 0 };
    int m_height{ 0 };
    std::vector<int> m_tiles;

    // Non-serialized
    // @TODO: make it an asset
    SpriteSheet m_sprite;

    mutable std::shared_ptr<GpuMesh> m_mesh;
};

}  // namespace my
