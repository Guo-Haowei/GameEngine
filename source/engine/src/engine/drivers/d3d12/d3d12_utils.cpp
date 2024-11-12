#if 0
#include <comdef.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include "DXUtils.h"
#include "core/framework/AssetManager.h"
#include "core/io/print.h"

namespace vct
{

// @TODO: remove
ID3DBlob* CompileShaderFromFile(const char* name, SHADER_TYPE type, bool debugShader, const char* entry)
{
    const char* target = "";
    switch (type)
    {
        case SHADER_TYPE_VERTEX:
            target = "vs_5_0";
            break;
        case SHADER_TYPE_PIXEL:
            target = "ps_5_0";
            break;
        default:
            PANIC("");
            break;
    }

    std::string path = g_assetManager->BuildPath(std::string("source/vct/src/vct/shaders/") + name);
    if (path.empty())
    {
        LOG_ERROR("shader '{}' does not exist", name);
        return nullptr;
    }

    std::string pathstr(path);
    std::wstring pathwstr(pathstr.begin(), pathstr.end());

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    if (debugShader)
    {
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    }

    ComPtr<ID3DBlob> errorBlob;
    ID3DBlob* blob = nullptr;
    const HRESULT hr = D3DCompileFromFile(pathwstr.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target, flags, 0, &blob, errorBlob.GetAddressOf());
    if (hr != S_OK)
    {
        [[maybe_unused]] const char* error = errorBlob ? reinterpret_cast<const char*>(errorBlob->GetBufferPointer()) : "";

        LOG_FATAL("(0x{:08x}): Failed to compile shader [{}]:\n{}", static_cast<uint32_t>(hr), name, error);
        return nullptr;
    }

    return blob;
}

}  // namespace vct
#endif