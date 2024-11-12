#pragma once

#define D3D_FAIL(HR)                 ERR_FAIL_COND_MSG(FAILED(HR), #HR)
#define D3D_FAIL_V(HR, RET)          ERR_FAIL_COND_V_MSG(FAILED(HR), RET, #HR)
#define D3D_FAIL_MSG(HR, MSG)        ERR_FAIL_COND_MSG(FAILED(HR), MSG)
#define D3D_FAIL_V_MSG(HR, RET, MSG) ERR_FAIL_COND_V_MSG(FAILED(HR), RET, MSG)
#define D3D_CALL(HR)                 CRASH_COND_MSG(FAILED(HR), #HR)

// @TODO: refactor
#define D3D_RETURN_IF_FAIL(expr)                           \
    if (HRESULT _hr = (expr); FAILED(_hr)) [[unlikely]] {  \
        return VCT_ERROR(_hr, "operation failed: " #expr); \
    } else                                                 \
        (void)0

namespace my {

template<class T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

}  // namespace my
