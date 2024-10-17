#include "uniform_buffer.h"

#include "core/framework/graphics_manager.h"

namespace my {

void UniformBufferBase::update(const void* p_data, size_t p_size) {
    auto& gm = GraphicsManager::singleton();
    switch (gm.getBackend()) {
        case Backend::OPENGL:
            gm.updateUniform(this, p_data, p_size);
            break;
        case Backend::D3D11:
            gm.updateUniform(this, p_data, p_size);
            gm.bindUniformRange(this, (uint32_t)p_size, 0);
            break;
        default:
            break;
    }
}

}  // namespace my
