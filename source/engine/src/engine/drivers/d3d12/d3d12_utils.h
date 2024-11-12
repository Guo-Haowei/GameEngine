#pragma once
#include <d3dcommon.h>
#include <dxgi.h>
#include <wrl/client.h>

// @TODO: refactor
#define D3D_RETURN_IF_FAIL(expr)                           \
    if (HRESULT _hr = (expr); FAILED(_hr)) [[unlikely]] {  \
        return VCT_ERROR(_hr, "operation failed: " #expr); \
    } else                                                 \
        (void)0

namespace my {

inline constexpr float kRedClearColor[4] = { 0.4f, 0.3f, 0.3f, 1.0f };
inline constexpr float kGreenClearColor[4] = { 0.3f, 0.4f, 0.3f, 1.0f };

template<class T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

}  // namespace my
