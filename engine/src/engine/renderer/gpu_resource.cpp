#include "gpu_resource.h"

#include "engine/renderer/base_graphics_manager.h"

namespace my {

// @TODO: remove this code
void GpuConstantBuffer::update(const void* p_data, size_t p_size) {
    auto& gm = IGraphicsManager::GetSingleton();
    switch (gm.GetBackend()) {
        case Backend::OPENGL:
            gm.UpdateConstantBuffer(this, p_data, p_size);
            break;
        case Backend::D3D11:
            gm.UpdateConstantBuffer(this, p_data, p_size);
            gm.BindConstantBufferRange(this, (uint32_t)p_size, 0);
            break;
        default:
            break;
    }
}

}  // namespace my
