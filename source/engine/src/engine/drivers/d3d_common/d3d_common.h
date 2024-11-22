#pragma once
#include <d3dcommon.h>
#include <wrl/client.h>

#include "rendering/gpu_resource.h"

#define USE_D3D_DEBUG_NAME USE_IF(USING(DEBUG_BUILD))

#define D3D_FAIL(HR)                 ERR_FAIL_COND_MSG(FAILED(HR), #HR)
#define D3D_FAIL_V(HR, RET)          ERR_FAIL_COND_V_MSG(FAILED(HR), RET, #HR)
#define D3D_FAIL_MSG(HR, MSG)        ERR_FAIL_COND_MSG(FAILED(HR), MSG)
#define D3D_FAIL_V_MSG(HR, RET, MSG) ERR_FAIL_COND_V_MSG(FAILED(HR), RET, MSG)
#define D3D_CALL(HR)                 CRASH_COND_MSG(FAILED(HR), #HR)

#if USING(USE_D3D_DEBUG_NAME)
#define D3D11_SET_DEBUG_NAME(RES, NAME) ::my::SetDebugName(RES, NAME)
#define D3D12_SET_DEBUG_NAME(RES, NAME) ::my::SetDebugName(RES, NAME)
#else
#define D3D11_SET_DEBUG_NAME(RES, NAME) ((void)0)
#define D3D12_SET_DEBUG_NAME(RES, NAME) ((void)0)
#endif

struct ID3D11DeviceChild;
struct ID3D12DeviceChild;

namespace my {

template<class T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

static inline D3D_SRV_DIMENSION ConvertDimension(Dimension p_dimension) {
    switch (p_dimension) {
        case Dimension::TEXTURE_2D:
            return D3D_SRV_DIMENSION_TEXTURE2D;
        case Dimension::TEXTURE_3D:
            return D3D_SRV_DIMENSION_TEXTURE3D;
        case Dimension::TEXTURE_2D_ARRAY:
            return D3D_SRV_DIMENSION_TEXTURE2DARRAY;
        case Dimension::TEXTURE_CUBE:
            return D3D_SRV_DIMENSION_TEXTURECUBE;
        case Dimension::TEXTURE_CUBE_ARRAY:
            return D3D_SRV_DIMENSION_TEXTURECUBEARRAY;
        default:
            CRASH_NOW();
            return D3D_SRV_DIMENSION_TEXTURE2D;
    }
}

auto CompileShader(std::string_view p_path,
                   const char* p_target,
                   const D3D_SHADER_MACRO* p_defines) -> std::expected<Microsoft::WRL::ComPtr<ID3DBlob>, ErrorRef>;

#if USING(USE_D3D_DEBUG_NAME)
void SetDebugName(ID3D11DeviceChild* p_resource, const std::string& p_name);
void SetDebugName(ID3D12DeviceChild* p_resource, const std::string& p_name);
#endif

}  // namespace my
