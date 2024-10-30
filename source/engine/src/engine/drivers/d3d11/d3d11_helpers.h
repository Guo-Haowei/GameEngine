#ifdef D3D11_HELPERS_H_INCLUDED
#error DO NOT INCLUDE THIS IN A HEADER FILE
#endif

#define D3D11_HELPERS_H_INCLUDED

#include <d3d11.h>

#include "rendering/gpu_resource.h"

#if USING(DEBUG_BUILD)
#pragma comment(lib, "dxguid.lib")
#endif

#define D3D_FAIL_MSG(HR, MSG)        ERR_FAIL_COND_MSG(FAILED(HR), MSG)
#define D3D_FAIL_V_MSG(HR, RET, MSG) ERR_FAIL_COND_V_MSG(FAILED(HR), RET, MSG)

#if USING(DEBUG_BUILD)
#define SET_DEBUG_NAME(RES, NAME) ::my::d3d::SetDebugName(RES, NAME)
#else
#define SET_DEBUG_NAME(RES, NAME) \
    do {                          \
        (void)RES, (void)NAME;    \
    } while (0)
#endif

namespace my::d3d {

void SetDebugName(ID3D11DeviceChild* p_resource, const std::string& p_name);

inline D3D_SRV_DIMENSION ConvertDimension(Dimension p_dimension) {
    switch (p_dimension) {
        case Dimension::TEXTURE_2D:
            return D3D_SRV_DIMENSION_TEXTURE2D;
        case Dimension::TEXTURE_3D:
            return D3D_SRV_DIMENSION_TEXTURE3D;
        case Dimension::TEXTURE_2D_ARRAY:
            return D3D_SRV_DIMENSION_TEXTURE2DARRAY;
        case Dimension::TEXTURE_CUBE:
            return D3D_SRV_DIMENSION_TEXTURECUBE;
        default:
            CRASH_NOW();
            return D3D_SRV_DIMENSION_TEXTURE2D;
    }
}

inline uint32_t ConvertResourceMiscFlags(uint32_t p_misc_flags) {
    // only support a few flags for now
    [[maybe_unused]] constexpr uint32_t supported_flags = RESOURCE_MISC_GENERATE_MIPS | RESOURCE_MISC_TEXTURECUBE;
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

inline uint32_t ConvertBindFlags(uint32_t p_bind_flags) {
    [[maybe_unused]] constexpr uint32_t supported_flags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_DEPTH_STENCIL | BIND_UNORDERED_ACCESS;
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
