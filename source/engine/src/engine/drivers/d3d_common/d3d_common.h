#pragma once

#define D3D_FAIL(HR)                 ERR_FAIL_COND_MSG(FAILED(HR), #HR)
#define D3D_FAIL_V(HR, RET)          ERR_FAIL_COND_V_MSG(FAILED(HR), RET, #HR)
#define D3D_FAIL_MSG(HR, MSG)        ERR_FAIL_COND_MSG(FAILED(HR), MSG)
#define D3D_FAIL_V_MSG(HR, RET, MSG) ERR_FAIL_COND_V_MSG(FAILED(HR), RET, MSG)
#define D3D_CALL(HR)                 CRASH_COND_MSG(FAILED(HR), #HR)

namespace my {

template<class T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

}  // namespace my
