#pragma once

#include "../opengl_common/opengl_common_graphics_manager.h"

struct GLFWwindow;

namespace my {

class OpenGL4GraphicsManager : public CommonOpenGlGraphicsManager {
public:
    OpenGL4GraphicsManager() : CommonOpenGlGraphicsManager() {}

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) final;
    void BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) final;
    void UnbindUnorderedAccessView(uint32_t p_slot) final;

    void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) final;
    void UnbindStructuredBuffer(int p_slot) final;

protected:
    auto InitializeInternal() -> Result<void> final;
};

}  // namespace my
