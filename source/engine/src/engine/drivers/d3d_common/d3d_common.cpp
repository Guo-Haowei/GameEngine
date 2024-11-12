#include "d3d_common.h"

#include <d3d11.h>
#include <d3d12.h>

#if USING(USE_D3D_DEBUG_NAME)
#pragma comment(lib, "dxguid.lib")
#endif

namespace my {

#if USING(USE_D3D_DEBUG_NAME)
void SetDebugName(ID3D11DeviceChild* p_resource, const std::string& p_name) {
    std::wstring name(p_name.begin(), p_name.end());
    p_resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size() * sizeof(wchar_t), name.c_str());
}

void SetDebugName(ID3D12DeviceChild* p_resource, const std::string& p_name) {
    std::wstring name(p_name.begin(), p_name.end());
    p_resource->SetName(name.c_str());
}
#endif

}  // namespace my
