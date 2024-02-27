#include "d3d11_helpers.h"

#if USING(DEBUG_BUILD)
#pragma comment(lib, "dxguid.lib")
#endif

namespace my::d3d11 {

void set_debug_name(ID3D11DeviceChild* p_resource, const std::string& p_name) {
    std::wstring w_name(p_name.begin(), p_name.end());

    p_resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)w_name.size() * sizeof(wchar_t), w_name.c_str());
}

DXGI_FORMAT convert_format(PixelFormat p_format) {
    switch (p_format) {
        case PixelFormat::UNKNOWN:
            return DXGI_FORMAT_UNKNOWN;
        case PixelFormat::R8_UINT:
            return DXGI_FORMAT_R8_UNORM;
        case PixelFormat::R8G8_UINT:
            return DXGI_FORMAT_R8G8_UNORM;
        case PixelFormat::R8G8B8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::R8G8B8A8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::R16_FLOAT:
            return DXGI_FORMAT_R16_FLOAT;
        case PixelFormat::R16G16_FLOAT:
            return DXGI_FORMAT_R16G16_FLOAT;
        case PixelFormat::R16G16B16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case PixelFormat::R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case PixelFormat::R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case PixelFormat::R32G32_FLOAT:
            return DXGI_FORMAT_R32G32_FLOAT;
        case PixelFormat::R32G32B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case PixelFormat::R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case PixelFormat::D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;
        default:
            CRASH_NOW();
            return DXGI_FORMAT_UNKNOWN;
    }
}

}  // namespace my::d3d11
