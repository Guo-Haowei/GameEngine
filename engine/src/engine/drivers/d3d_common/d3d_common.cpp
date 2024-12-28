#include "d3d_common.h"

#include <d3d11.h>
#include <d3d12.h>
#include <d3dcompiler.h>

#include <fstream>

#include "engine/core/framework/asset_registry.h"
#include "engine/core/string/string_builder.h"

#if USING(USE_D3D_DEBUG_NAME)
#pragma comment(lib, "dxguid.lib")
#endif

namespace my {

namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;

HRESULT ReportErrorIfFailed(HRESULT p_result,
                            std::string_view p_function,
                            std::string_view p_file,
                            int p_line,
                            std::string_view p_error) {
    if (FAILED(p_result)) {
        ReportErrorImpl(p_function, p_file, p_line, p_error);
        // GENERATE_TRAP();
    }
    return p_result;
}

class D3DIncludeHandler : public ID3DInclude {
public:
    STDMETHOD(Open)
    (D3D_INCLUDE_TYPE, LPCSTR p_file, LPCVOID, LPCVOID* p_out_data, UINT* p_bytes) override {
        // @TODO: fix search
        fs::path path = fs::path{ ROOT_FOLDER } / "engine/shader/" / p_file;

        auto res = AssetRegistry::GetSingleton().RequestAssetSync(path.string());
        if (!res) {
            StringStreamBuilder builder;
            builder << res.error();
            LOG_ERROR("{}", builder.ToString());
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        auto source_binary = dynamic_cast<const BufferAsset*>(*res);
        if (source_binary->buffer.empty()) {
            LOG_ERROR("failed to read file '{}'", path.string());
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
                   const D3D_SHADER_MACRO* p_defines) -> Result<ComPtr<ID3DBlob>> {
    fs::path name = fs::path("engine") / "shader" / "hlsl" / (std::string(p_path) + ".hlsl");
    fs::path fullpath = fs::path{ ROOT_FOLDER } / name;
    std::string fullpath_str = fullpath.string();

    std::wstring path{ fullpath_str.begin(), fullpath_str.end() };
    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> source;

    D3DIncludeHandler include_handler;

    uint32_t flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if 0
#if USING(DEBUG_BUILD)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
#endif

    if (!fs::exists(fullpath_str)) {
        return HBN_ERROR(ErrorCode::ERR_FILE_NOT_FOUND, "file '{}' not found", name.string());
    }

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
        std::string error_message = std::format("failed to compile shader '{}'", fullpath_str);
        if (error != nullptr) {
            error_message.append(", details: ");
            error_message.append((const char*)error->GetBufferPointer());
        }
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "{}", error_message);
    }

    return source;
}

#if USING(USE_D3D_DEBUG_NAME)
void SetDebugName(ID3D11DeviceChild* p_resource, const std::string& p_name) {
    p_resource->SetPrivateData(WKPDID_D3DDebugObjectName, (uint32_t)p_name.length(), p_name.c_str());
}

void SetDebugName(ID3D12DeviceChild* p_resource, const std::string& p_name) {
    std::wstring name(p_name.begin(), p_name.end());
    p_resource->SetName(name.c_str());
}
#endif

}  // namespace my
