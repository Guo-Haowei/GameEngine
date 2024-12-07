#pragma once
#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/sampler.h"

// @TODO: remove this class
#include "engine/core/framework/graphics_manager.h"

namespace my {

class Scene;

using PointShadowHandle = int;
constexpr PointShadowHandle INVALID_POINT_SHADOW_HANDLE = -1;

class RenderManager : public Singleton<RenderManager>, public Module {
public:
    RenderManager();

    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void update(Scene&) {}

    PointShadowHandle allocate_point_light_shadowMap();
    void free_point_light_shadowMap(PointShadowHandle& p_handle);

private:
    std::list<PointShadowHandle> m_free_point_light_shadow;
};

}  // namespace my

namespace my::renderer {

void register_rendering_dvars();

void fill_texture_and_sampler_desc(const ImageAsset* p_image, GpuTextureDesc& p_texture_desc, SamplerDesc& p_sampler_desc);

}  // namespace my::renderer
