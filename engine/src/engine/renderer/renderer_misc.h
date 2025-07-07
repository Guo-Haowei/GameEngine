#pragma once
#include "gpu_resource.h"

namespace my {

template<typename T>
static GpuBufferDesc CreateDesc(const std::vector<T>& p_data) {
    GpuBufferDesc desc{
        .elementSize = sizeof(T),
        .elementCount = static_cast<uint32_t>(p_data.size()),
        .offset = 0,
        .initialData = p_data.data(),
    };
    return desc;
}

}  // namespace my
