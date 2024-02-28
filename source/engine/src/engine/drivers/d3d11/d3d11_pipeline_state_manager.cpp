#include "d3d11_pipeline_state_manager.h"

#include <d3dcompiler.h>

#include "drivers/d3d11/convert.h"
#include "drivers/d3d11/d3d11_helpers.h"

extern ID3D11Device* get_d3d11_device();

namespace my {

namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;

static auto compile_shader(std::string_view p_path, const char* p_target, const D3D_SHADER_MACRO* p_defines) -> std::expected<ComPtr<ID3DBlob>, std::string> {
    fs::path fullpath = fs::path{ ROOT_FOLDER } / "source" / "shader" / "hlsl" / (std::string(p_path) + ".hlsl");
    std::string fullpath_str = fullpath.string();

    std::wstring path{ fullpath_str.begin(), fullpath_str.end() };
    ComPtr<ID3DBlob> error;
    ComPtr<ID3DBlob> source;

    uint32_t flags = D3DCOMPILE_ENABLE_STRICTNESS;
    HRESULT hr = D3DCompileFromFile(
        path.c_str(),
        p_defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
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
            return std::unexpected("");
        }
    }

    return source;
}

std::shared_ptr<PipelineState> D3d11PipelineStateManager::create(const PipelineCreateInfo& p_info) {
    ID3D11Device* device = get_d3d11_device();
    DEV_ASSERT(device);
    if (!device) {
        return nullptr;
    }
    auto pipeline_state = std::make_shared<D3d11PipelineState>();

    HRESULT hr = S_OK;

    std::vector<D3D_SHADER_MACRO> macros;
    for (const auto& define : p_info.defines) {
        macros.push_back({ define.name, define.value });
    }
    macros.push_back({ "HLSL_LANG", "1" });
    macros.push_back({ nullptr, nullptr });

    ComPtr<ID3DBlob> vsblob;
    if (!p_info.vs.empty()) {
        auto res = compile_shader(p_info.vs, "vs_5_0", macros.data());
        if (!res) {
            LOG_ERROR("Failed to compile '{}'\n  detail: {}", p_info.vs, res.error());
            return nullptr;
        }

        ComPtr<ID3DBlob> blob;
        blob = *res;
        hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pipeline_state->vertex_shader.GetAddressOf());
        D3D_FAIL_V_MSG(hr, nullptr, "failed to create vertex buffer");
        vsblob = blob;
    }
    if (!p_info.ps.empty()) {
        auto res = compile_shader(p_info.ps, "ps_5_0", macros.data());
        if (!res) {
            LOG_ERROR("Failed to compile '{}'\n  detail: {}", p_info.vs, res.error());
            return nullptr;
        }

        ComPtr<ID3DBlob> blob;
        blob = *res;
        hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pipeline_state->pixel_shader.GetAddressOf());
        D3D_FAIL_V_MSG(hr, nullptr, "failed to create vertex buffer");
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> elements;
    elements.reserve(p_info.input_layout_desc->elements.size());
    for (const auto& ele : p_info.input_layout_desc->elements) {
        D3D11_INPUT_ELEMENT_DESC ildesc;
        ildesc.SemanticName = ele.semanticName.c_str();
        ildesc.SemanticIndex = ele.semanticIndex;
        ildesc.Format = convert(ele.format);
        ildesc.InputSlot = ele.inputSlot;
        ildesc.AlignedByteOffset = ele.alignedByteOffset;
        ildesc.InputSlotClass = convert(ele.inputSlotClass);
        ildesc.InstanceDataStepRate = ele.instanceDataStepRate;
        elements.emplace_back(ildesc);
    }
    DEV_ASSERT(elements.size());

    hr = device->CreateInputLayout(elements.data(), (UINT)elements.size(), vsblob->GetBufferPointer(), vsblob->GetBufferSize(), pipeline_state->input_layout.GetAddressOf());
    D3D_FAIL_V_MSG(hr, nullptr, "failed to create input layout");

    return pipeline_state;
}

}  // namespace my

#if 0
void HlslProgram::CompileShader(string const& file, LPCSTR entry, LPCSTR target, ComPtr<ID3DBlob>& sourceBlob) {
    string fileStr(file);
    wstring fileWStr(fileStr.begin(), fileStr.end());
    ComPtr<ID3DBlob> errorBlob;
}

void HlslProgram::create(ComPtr<ID3D11Device>& device, const char* debugName, char const* vertName, const char* fragName) {
    string path(HLSL_DIR);
    string vertFile(vertName);
    vertFile.append(".vert.hlsl");
    string pixelFile(fragName == nullptr ? vertName : fragName);
    pixelFile.append(".pixel.hlsl");
    SHADER_COMPILING_START_INFO(debugName);
    HlslProgram::CompileShader(path + vertFile, "vs_main", "vs_5_0", vertShaderBlob);
    HRESULT hr = device->CreateVertexShader(
        vertShaderBlob->GetBufferPointer(),
        vertShaderBlob->GetBufferSize(),
        NULL,
        vertShader.GetAddressOf());
    D3D_THROW_IF_FAILED(hr, "Failed to create vertex shader");
    ComPtr<ID3DBlob> pixelBlob;
    HlslProgram::CompileShader(path + pixelFile, "ps_main", "ps_5_0", pixelBlob);
    hr = device->CreatePixelShader(
        pixelBlob->GetBufferPointer(),
        pixelBlob->GetBufferSize(),
        NULL,
        pixelShader.GetAddressOf());
    D3D_THROW_IF_FAILED(hr, "Failed to create pixel shader");
    SHADER_COMPILING_END_INFO(debugName);
}
#endif