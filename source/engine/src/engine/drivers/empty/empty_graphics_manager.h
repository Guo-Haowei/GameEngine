#pragma once
#include "core/framework/graphics_manager.h"

namespace my {

class EmptyGraphicsManager : public GraphicsManager {
public:
    EmptyGraphicsManager(Backend p_backend) : GraphicsManager("EmptyGraphicsManager", p_backend) {}

    bool initialize() override { return true; }
    void finalize() override {}
    void render() override {}

    std::shared_ptr<Texture> create_texture(const TextureDesc&, const SamplerDesc&) { return nullptr; }

    std::shared_ptr<Subpass> create_subpass(const SubpassDesc&) override { return nullptr; }

    uint64_t get_final_image() const override { return 0; }

    // @TODO: refactor this
    void fill_material_constant_buffer(const MaterialComponent*, MaterialConstantBuffer&) override {}

protected:
    void on_scene_change(const Scene&) override {}
    void set_pipeline_state_impl(PipelineStateName) override {}
};

}  // namespace my
