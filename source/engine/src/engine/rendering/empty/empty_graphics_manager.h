#pragma once
#include "core/framework/graphics_manager.h"

namespace my {
class EmptyGraphicsManager : public GraphicsManager {
public:
    EmptyGraphicsManager() : GraphicsManager("EmptyGraphicsManager") {}

    bool initialize() final { return true; }
    void finalize() final {}
    void render() final {}

    // @TODO: filter
    void create_texture(ImageHandle*) final {}

    uint32_t get_final_image() const final { return 0; }

    std::shared_ptr<RenderTarget> create_resource(const RenderTargetDesc&) final { return nullptr; }
    std::shared_ptr<RenderTarget> find_resource(const std::string&) const final { return nullptr; }

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent*, MaterialConstantBuffer&) final {}

protected:
    void on_scene_change(const Scene&) override {}
};

}  // namespace my
