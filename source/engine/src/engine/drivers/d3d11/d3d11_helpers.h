#ifdef D3D11_HELPERS_H_INCLUDED
#error DO NOT INCLUDE THIS IN A HEADER FILE
#endif

#define D3D11_HELPERS_H_INCLUDED

#include <d3d11.h>

#include "rendering/gpu_resource.h"

namespace my::d3d {

inline uint32_t ConvertResourceMiscFlags(ResourceMiscFlags p_misc_flags) {
    // only support a few flags for now
    [[maybe_unused]] constexpr ResourceMiscFlags supported_flags = RESOURCE_MISC_GENERATE_MIPS | RESOURCE_MISC_TEXTURECUBE;
    DEV_ASSERT((p_misc_flags & (~supported_flags)) == 0);

    uint32_t flags = 0;
    if (p_misc_flags & RESOURCE_MISC_GENERATE_MIPS) {
        flags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    if (p_misc_flags & RESOURCE_MISC_TEXTURECUBE) {
        flags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    }

    return flags;
}

inline uint32_t ConvertBindFlags(BindFlags p_bind_flags) {
    [[maybe_unused]] constexpr BindFlags supported_flags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_DEPTH_STENCIL | BIND_UNORDERED_ACCESS;
    DEV_ASSERT((p_bind_flags & (~supported_flags)) == 0);

    uint32_t flags = 0;
    if (p_bind_flags & BIND_SHADER_RESOURCE) {
        flags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (p_bind_flags & BIND_RENDER_TARGET) {
        flags |= D3D11_BIND_RENDER_TARGET;
    }
    if (p_bind_flags & BIND_DEPTH_STENCIL) {
        flags |= D3D11_BIND_DEPTH_STENCIL;
    }
    if (p_bind_flags & BIND_UNORDERED_ACCESS) {
        flags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    return flags;
}

inline uint32_t ConvertCpuAccessFlags(uint32_t p_access_flags) {
    [[maybe_unused]] constexpr uint32_t supported_flags = CPU_ACCESS_READ | CPU_ACCESS_WRITE;
    DEV_ASSERT((p_access_flags & (~supported_flags)) == 0);

    uint32_t flags = 0;
    if (p_access_flags & CPU_ACCESS_READ) {
        flags |= D3D11_CPU_ACCESS_READ;
    }
    if (p_access_flags & CPU_ACCESS_WRITE) {
        flags |= D3D11_CPU_ACCESS_WRITE;
    }

    return flags;
}

}  // namespace my::d3d
