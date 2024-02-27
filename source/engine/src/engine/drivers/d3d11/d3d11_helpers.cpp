#include "d3d11_helpers.h"

#include "drivers/d3d11/d3d11_graphics_manager.h"

ID3D11Device* get_d3d11_device() {
    auto graphics_manager = dynamic_cast<my::D3d11GraphicsManager*>(my::GraphicsManager::singleton_ptr());
    if (!graphics_manager) {
        return nullptr;
    }
    return graphics_manager->get_device();
}

namespace my::d3d11 {

void set_debug_name(ID3D11DeviceChild* p_resource, const std::string& p_name) {
    std::wstring w_name(p_name.begin(), p_name.end());

    p_resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)w_name.size() * sizeof(wchar_t), w_name.c_str());
}

}  // namespace my::d3d11
