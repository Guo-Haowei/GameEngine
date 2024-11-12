#include "d3d_common.h"

#include <d3d11.h>
#include <d3d12.h>
#include <d3dcompiler.h>

#include <fstream>

#include "core/framework/asset_manager.h"

#if USING(USE_D3D_DEBUG_NAME)
#pragma comment(lib, "dxguid.lib")
#endif

namespace my {

namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;

class D3DIncludeHandler : public ID3DInclude {
public:
    STDMETHOD(Open)
    (D3D_INCLUDE_TYPE, LPCSTR p_file, LPCVOID, LPCVOID* p_out_data, UINT* p_bytes) override {
        // @TODO: fix search
        FilePath path = FilePath{ ROOT_FOLDER } / "source/shader/" / p_file;

        auto source_binary = AssetManager::GetSingleton().LoadFileSync(path);
        if (!source_binary || source_binary->buffer.empty()) {
            LOG_ERROR("failed to read file '{}'", path.String());
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        *p_out_data = source_binary->buffer.data();
        *p_bytes = (UINT)source_binary->buffer.size();
        return S_OK;
    }

    STDMETHOD(Close)
    (LPCVOID) override {
        return S_OK;
    }
};

auto CompileShader(std::string_view p_path,
                   const char* p_target,
                   const D3D_SHADER_MACRO* p_defines) -> std::expected<ComPtr<ID3DBlob>, std::string> {
    fs::path fullpath = fs::path{ ROOT_FOLDER } / "source" / "shader" / "hlsl" / (std::string(p_path) + ".hlsl");
    std::string fullpath_str = fullpath.string();

    std::wstring path{ fullpath_str.begin(), fullpath_str.end() };
    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> source;

    D3DIncludeHandler include_handler;

    uint32_t flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompileFromFile(
        path.c_str(),
        p_defines,
        &include_handler,
        "main",
        p_target,
        flags,
        0,
        source.GetAddressOf(),
        error.GetAddressOf());

    if (FAILED(hr)) {
        if (error != nullptr) {
            return std::unexpected(std::string((const char*)error->GetBufferPointer()));
        } else {
            return std::unexpected("Unkown error");
        }
    }

    return source;
}

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
