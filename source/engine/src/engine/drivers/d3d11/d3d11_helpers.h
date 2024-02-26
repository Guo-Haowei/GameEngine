#ifdef D3D11_HELPERS_H_INCLUDED
#error DO NOT INCLUDE THIS IN A HEADER FILE
#endif

#define D3D11_HELPERS_H_INCLUDED

#include <d3d11.h>

#include "rendering/pixel_format.h"

#define D3D_FAIL_MSG(HR, MSG)        ERR_FAIL_COND_MSG(FAILED(HR), MSG)
#define D3D_FAIL_V_MSG(HR, RET, MSG) ERR_FAIL_COND_V_MSG(FAILED(HR), RET, MSG)

#if USING(DEBUG_BUILD)
#define SET_DEBUG_NAME(RES, NAME) ::my::d3d11::set_debug_name(RES, NAME)
#else
#define SET_DEBUG_NAME(RES, NAME) \
    do {                          \
        (void)RES, (void)NAME;    \
    } while (0)
#endif

namespace my::d3d11 {

void set_debug_name(ID3D11DeviceChild* p_resource, const std::string& p_name);

inline DXGI_FORMAT convert_format(PixelFormat p_format) {
    switch (p_format) {
        case FORMAT_UNKNOWN:
            return DXGI_FORMAT_UNKNOWN;
        case FORMAT_R8_UINT:
            return DXGI_FORMAT_R8_UNORM;
        case FORMAT_R8G8_UINT:
            return DXGI_FORMAT_R8G8_UNORM;
        case FORMAT_R8G8B8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case FORMAT_R8G8B8A8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case FORMAT_R16_FLOAT:
            return DXGI_FORMAT_R16_FLOAT;
        case FORMAT_R16G16_FLOAT:
            return DXGI_FORMAT_R16G16_FLOAT;
        case FORMAT_R16G16B16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case FORMAT_R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case FORMAT_R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case FORMAT_R32G32_FLOAT:
            return DXGI_FORMAT_R32G32_FLOAT;
        case FORMAT_R32G32B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case FORMAT_R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case FORMAT_D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;
        default:
            CRASH_NOW();
            return DXGI_FORMAT_UNKNOWN;
    }
}

}  // namespace my::d3d11
