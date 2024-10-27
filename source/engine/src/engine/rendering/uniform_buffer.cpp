#include "uniform_buffer.h"

#include "core/framework/graphics_manager.h"

namespace my {

void UniformBufferBase::update(const void* p_data, size_t p_size) {
    auto& gm = GraphicsManager::singleton();
    switch (gm.GetBackend()) {
        case Backend::OPENGL:
            gm.UpdateUniform(this, p_data, p_size);
            break;
        case Backend::D3D11:
            gm.UpdateUniform(this, p_data, p_size);
            gm.BindUniformRange(this, (uint32_t)p_size, 0);
            break;
        default:
            break;
    }
}

}  // namespace my
