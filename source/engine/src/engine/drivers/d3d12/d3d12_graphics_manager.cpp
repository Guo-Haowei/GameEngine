#include "d3d12_graphics_manager.h"

#include "drivers/d3d12/d3d12_pipeline_state_manager.h"

namespace my {

D3d12GraphicsManager::D3d12GraphicsManager() : EmptyGraphicsManager("D3d12GraphicsManager", Backend::D3D12) {
    m_pipelineStateManager = std::make_shared<D3d12PipelineStateManager>();
}

ID3D12CommandQueue* D3d12GraphicsManager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE p_type) {
    D3D12_COMMAND_QUEUE_DESC desc;
    desc.Type = p_type;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

    ID3D12CommandQueue* queue;
    HRESULT hr = WIN_CALL(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
    if (FAILED(hr)) {
        return nullptr;
    }

    const wchar_t* debugName = nullptr;
    switch (p_type) {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            debugName = L"GraphicsCommandQueue";
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            debugName = L"ComputeCommandQueue";
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            debugName = L"CopyCommandQueue";
            break;
        default:
            CRASH_NOW();
            break;
    }

    NAME_DX12_OBJECT(queue, debugName);
    return queue;
}

}  // namespace my
