#include "uniform_buffer.h"

#include "core/framework/graphics_manager.h"

namespace my {

void UniformBufferBase::update(const void* p_data, size_t p_size) {
    GraphicsManager::singleton().uniform_update(this, p_data, p_size);
}

}  // namespace my
