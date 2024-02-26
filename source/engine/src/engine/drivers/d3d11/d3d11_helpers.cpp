#include "d3d11_helpers.h"

#if USING(DEBUG_BUILD)
#pragma comment(lib, "dxguid.lib")
#endif

namespace my::d3d11 {

void set_debug_name(ID3D11DeviceChild* p_resource, const std::string& p_name) {
    std::wstring w_name(p_name.begin(), p_name.end());

    p_resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)w_name.size() * sizeof(wchar_t), w_name.c_str());
}

}  // namespace my::d3d11
