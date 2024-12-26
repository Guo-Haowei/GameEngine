#include "d3d12_graphics_manager.h"

#include <imgui/backends/imgui_impl_dx12.h>

#include "engine/core/framework/application.h"
#include "engine/core/framework/imgui_manager.h"
#include "engine/core/string/string_utils.h"
#include "engine/drivers/d3d12/d3d12_pipeline_state_manager.h"
#include "engine/drivers/d3d_common/d3d_common.h"
#include "engine/drivers/windows/win32_display_manager.h"
#include "engine/renderer/graphics_private.h"
#include "engine/scene/scene.h"
#include "structured_buffer.hlsl.h"

// @TODO: refactor
#include "engine/renderer/render_graph/pass_creator.h"

#define INCLUDE_AS_D3D12
#include "engine/drivers/d3d_common/d3d_convert.h"

namespace my {

using Microsoft::WRL::ComPtr;

// @TODO: refactor
struct D3d12GpuTexture : public GpuTexture {
    using GpuTexture::GpuTexture;

    uint64_t GetHandle() const final { return srvHandle.gpuHandle.ptr; }

    uint64_t GetResidentHandle() const final {
        uint64_t handle = srvHandle.index;
        switch (desc.dimension) {
            case Dimension::TEXTURE_2D:
            case Dimension::TEXTURE_CUBE_ARRAY:
                return handle;
            default:
                CRASH_NOW();
                return 0;
        }
    }

    uint64_t GetUavHandle() const final { return uavHandle.index; }

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;

    DescriptorHeapHandle srvHandle;
    DescriptorHeapHandle uavHandle;
};

struct D3d12Framebuffer : public Framebuffer {
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> dsvs;
};

struct D3d12ConstantBuffer : public GpuConstantBuffer {
    using GpuConstantBuffer::GpuConstantBuffer;

    ~D3d12ConstantBuffer() {
        if (buffer) {
            buffer->Unmap(0, nullptr);
        }
        mappedData = nullptr;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> buffer{};
    uint8_t* mappedData{ nullptr };
};

struct D3d12StructuredBuffer : public GpuStructuredBuffer {
    using GpuStructuredBuffer::GpuStructuredBuffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> buffer{};
    DescriptorHeapHandle handle;
};

struct D3d12FrameContext : FrameContext {
    ~D3d12FrameContext() {
        SafeRelease(m_commandAllocator);
    }

    void Wait(HANDLE p_fence_event, ID3D12Fence1* p_fence) {
        if (p_fence->GetCompletedValue() < m_fenceValue) {
            D3D_CALL(p_fence->SetEventOnCompletion(m_fenceValue, p_fence_event));
            WaitForSingleObject(p_fence_event, INFINITE);
        }
    }

    ID3D12CommandAllocator* m_commandAllocator = nullptr;
    uint64_t m_fenceValue = 0;
};

D3d12GraphicsManager::D3d12GraphicsManager() : GraphicsManager("D3d12GraphicsManager", Backend::D3D12, NUM_FRAMES_IN_FLIGHT) {
    m_pipelineStateManager = std::make_shared<D3d12PipelineStateManager>();
}

auto D3d12GraphicsManager::InitializeInternal() -> Result<void> {

    auto [w, h] = DisplayManager::GetSingleton().GetWindowSize();
    DEV_ASSERT(w > 0 && h > 0);

    if (auto res = CreateDevice(); !res) {
        return HBN_ERROR(res.error());
    }

    if (m_enableValidationLayer) {
        if (EnableDebugLayer()) {
            LOG("[GraphicsManager_DX12] Debug layer enabled");
        } else {
            LOG_ERROR("[GraphicsManager_DX12] Debug layer not enabled");
        }
    }

    if (auto res = CreateDescriptorHeaps(); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = InitGraphicsContext(); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = m_copyContext.Initialize(this); !res) {
        return HBN_ERROR(res.error());
    }

    for (int i = 0; i < NUM_BACK_BUFFERS; i++) {
        auto handle = m_rtvDescHeap.AllocHandle();
        m_renderTargetDescriptor[i] = handle.cpuHandle;
    }

    {
        auto handle = m_dsvDescHeap.AllocHandle();
        m_depthStencilDescriptor = handle.cpuHandle;
    }

    if (auto res = CreateSwapChain(w, h); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = CreateRenderTarget(w, h); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = CreateRootSignature(); !res) {
        return HBN_ERROR(res.error());
    }

    // Create debug buffer.
    {
        size_t bufferSize = sizeof(Vector4f) * 1000;  // hard code
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        D3D_CALL(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_debugVertexData)));
    }
    {
        size_t bufferSize = sizeof(uint32_t) * 3000;  // hard code
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        D3D_CALL(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_debugIndexData)));
    }

    auto imgui = m_app->GetImguiManager();
    if (imgui) {
        imgui->SetRenderCallbacks(
            [this]() {
                ImGui_ImplDX12_Init(m_device.Get(),
                                    NUM_FRAMES_IN_FLIGHT,
                                    d3d::Convert(DEFAULT_SURFACE_FORMAT),
                                    m_srvDescHeap.GetHeap(),
                                    m_srvDescHeap.GetStartCpu(),
                                    m_srvDescHeap.GetStartGpu());
                ImGui_ImplDX12_NewFrame();
            },
            []() {
                ImGui_ImplDX12_Shutdown();
            });
    }

    return Result<void>();
}

void D3d12GraphicsManager::FinalizeImpl() {
    if (m_initialized) {
        CleanupRenderTarget();

        FinalizeGraphicsContext();
        m_copyContext.Finalize();
    }
}

void D3d12GraphicsManager::Render() {
    ID3D12GraphicsCommandList* cmd_list = m_graphicsCommandList.Get();

    const auto [width, height] = DisplayManager::GetSingleton().GetWindowSize();
    CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    cmd_list->RSSetViewports(1, &viewport);
    D3D12_RECT rect{ 0, 0, width, height };
    cmd_list->RSSetScissorRects(1, &rect);

    // bind the frame buffer
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_depthStencilDescriptor;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = m_renderTargetDescriptor[m_backbufferIndex];

    // transfer resource state
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backbufferIndex],
                                                        D3D12_RESOURCE_STATE_PRESENT,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmd_list->ResourceBarrier(1, &barrier);

    cmd_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
    cmd_list->ClearRenderTargetView(rtv_handle, DEFAULT_CLEAR_COLOR, 0, nullptr);
    cmd_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // @TODO: refactor this
    if (m_app->IsRuntime())
        renderer::RenderGraphBuilder::DrawDebugImages(*renderer::GetRenderData(),
                                                      width,
                                                      height,
                                                      *this);

    if (m_app->GetSpecification().enableImgui) {
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list);
    }

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backbufferIndex],
                                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                   D3D12_RESOURCE_STATE_PRESENT);
    cmd_list->ResourceBarrier(1, &barrier);
}

void D3d12GraphicsManager::Present() {
    if (m_app->GetSpecification().enableImgui) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
    D3D_CALL(m_swapChain->Present(1, 0));  // Present with vsync
}

void D3d12GraphicsManager::BeginFrame() {
    // @TODO: wait for swap chain
    D3d12FrameContext& frame = reinterpret_cast<D3d12FrameContext&>(GetCurrentFrame());
    frame.Wait(m_graphicsFenceEvent, m_graphicsQueueFence.Get());
    D3D_CALL(frame.m_commandAllocator->Reset());
    D3D_CALL(m_graphicsCommandList->Reset(frame.m_commandAllocator, nullptr));

    WaitForSingleObject(m_swapChainWaitObject, INFINITE);

    m_backbufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    m_graphicsCommandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_graphicsCommandList->SetComputeRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heap = m_srvDescHeap.GetHeap();
    m_graphicsCommandList->SetDescriptorHeaps(1, &heap);

    // @TODO: NO HARDCODE
    CD3DX12_GPU_DESCRIPTOR_HANDLE handle{ m_srvDescHeap.GetStartGpu() };
    m_graphicsCommandList->SetGraphicsRootDescriptorTable(7, handle);
    m_graphicsCommandList->SetComputeRootDescriptorTable(7, handle);
}

void D3d12GraphicsManager::EndFrame() {
    D3D_CALL(m_graphicsCommandList->Close());
    ID3D12CommandList* cmdLists[] = { m_graphicsCommandList.Get() };
    m_graphicsCommandQueue->ExecuteCommandLists(array_length(cmdLists), cmdLists);
}

void D3d12GraphicsManager::MoveToNextFrame() {
    uint64_t fenceValue = m_lastSignaledFenceValue + 1;
    m_graphicsCommandQueue->Signal(m_graphicsQueueFence.Get(), fenceValue);
    m_lastSignaledFenceValue = fenceValue;

    D3d12FrameContext& frame = reinterpret_cast<D3d12FrameContext&>(GetCurrentFrame());
    frame.m_fenceValue = fenceValue;
    m_frameIndex = (m_frameIndex + 1) % static_cast<uint32_t>(m_frameContexts.size());
}

std::unique_ptr<FrameContext> D3d12GraphicsManager::CreateFrameContext() {
    return std::make_unique<D3d12FrameContext>();
}

void D3d12GraphicsManager::SetStencilRef(uint32_t p_ref) {
    m_graphicsCommandList->OMSetStencilRef(p_ref);
}

void D3d12GraphicsManager::SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) {
    unused(p_desc);
    unused(p_factor);
    unused(p_mask);
}

void D3d12GraphicsManager::SetRenderTarget(const Framebuffer* p_framebuffer, int p_index, int p_mip_level) {
    unused(p_mip_level);
    DEV_ASSERT(p_framebuffer);

    ID3D12GraphicsCommandList* command_list = m_graphicsCommandList.Get();

    auto framebuffer = reinterpret_cast<const D3d12Framebuffer*>(p_framebuffer);
    if (const auto depth_attachment = framebuffer->desc.depthAttachment; depth_attachment) {
        if (depth_attachment->desc.type == AttachmentType::SHADOW_CUBE_ARRAY) {
            D3D12_CPU_DESCRIPTOR_HANDLE dsv{ framebuffer->dsvs[p_index] };
            command_list->OMSetRenderTargets(0, nullptr, false, &dsv);
            return;
        }
    }

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
    for (auto& rtv : framebuffer->rtvs) {
        rtvs.emplace_back(rtv);
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv_ptr = nullptr;
    if (!framebuffer->dsvs.empty()) {
        dsv_ptr = &(framebuffer->dsvs[0]);
    }
    command_list->OMSetRenderTargets((uint32_t)rtvs.size(), rtvs.data(), false, dsv_ptr);
}

void D3d12GraphicsManager::UnsetRenderTarget() {
}

void D3d12GraphicsManager::BeginDrawPass(const Framebuffer* p_framebuffer) {
    ID3D12GraphicsCommandList* command_list = m_graphicsCommandList.Get();
    for (auto& texture : p_framebuffer->outSrvs) {
        D3D12_RESOURCE_STATES resource_state{};
        if (texture->desc.bindFlags & BIND_RENDER_TARGET) {
            resource_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        } else if (texture->desc.bindFlags & BIND_DEPTH_STENCIL) {
            resource_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        } else {
            CRASH_NOW();
        }

        auto d3d_texture = reinterpret_cast<D3d12GpuTexture*>(texture.get());
        auto barriers = CD3DX12_RESOURCE_BARRIER::Transition(d3d_texture->texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, resource_state);
        command_list->ResourceBarrier(1, &barriers);
    }
}

void D3d12GraphicsManager::EndDrawPass(const Framebuffer* p_framebuffer) {
    UnsetRenderTarget();
    ID3D12GraphicsCommandList* command_list = m_graphicsCommandList.Get();
    for (auto& texture : p_framebuffer->outSrvs) {
        D3D12_RESOURCE_STATES resource_state{};
        if (texture->desc.bindFlags & BIND_RENDER_TARGET) {
            resource_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        } else if (texture->desc.bindFlags & BIND_DEPTH_STENCIL) {
            resource_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        } else {
            CRASH_NOW();
        }

        auto d3d_texture = reinterpret_cast<D3d12GpuTexture*>(texture.get());
        auto barriers = CD3DX12_RESOURCE_BARRIER::Transition(d3d_texture->texture.Get(), resource_state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        command_list->ResourceBarrier(1, &barriers);
    }
}

void D3d12GraphicsManager::Clear(const Framebuffer* p_framebuffer, ClearFlags p_flags, const float* p_clear_color, int p_index) {
    auto framebuffer = reinterpret_cast<const D3d12Framebuffer*>(p_framebuffer);

    if (p_flags & CLEAR_COLOR_BIT) {
        DEV_ASSERT(p_clear_color);
        for (auto& rtv : framebuffer->rtvs) {
            m_graphicsCommandList->ClearRenderTargetView(rtv, p_clear_color, 0, nullptr);
        }
    }

    D3D12_CLEAR_FLAGS clear_flags{ static_cast<D3D12_CLEAR_FLAGS>(0) };
    if (p_flags & CLEAR_DEPTH_BIT) {
        clear_flags |= D3D12_CLEAR_FLAG_DEPTH;
    }
    if (p_flags & CLEAR_STENCIL_BIT) {
        clear_flags |= D3D12_CLEAR_FLAG_STENCIL;
    }
    if (clear_flags) {
        // @TODO: better way?
        DEV_ASSERT_INDEX(p_index, framebuffer->dsvs.size());
        m_graphicsCommandList->ClearDepthStencilView(framebuffer->dsvs[p_index], clear_flags, 1.0f, 0, 0, nullptr);
    }
}

void D3d12GraphicsManager::SetViewport(const Viewport& p_viewport) {
    CD3DX12_VIEWPORT viewport(
        static_cast<float>(p_viewport.topLeftX),
        static_cast<float>(p_viewport.topLeftY),
        static_cast<float>(p_viewport.width),
        static_cast<float>(p_viewport.height));

    D3D12_RECT rect{};
    rect.left = 0;
    rect.top = 0;
    rect.right = p_viewport.topLeftX + p_viewport.width;
    rect.bottom = p_viewport.topLeftY + p_viewport.height;

    m_graphicsCommandList->RSSetViewports(1, &viewport);
    m_graphicsCommandList->RSSetScissorRects(1, &rect);
}

ID3D12Resource* D3d12GraphicsManager::UploadBuffer(uint32_t p_byte_size, const void* p_init_data, ID3D12Resource* p_out_buffer) {
    if (p_byte_size == 0) {
        DEV_ASSERT(p_init_data == nullptr);
        return nullptr;
    }

    if (p_out_buffer == nullptr) {
        auto heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(p_byte_size);
        // Create the actual default buffer resource.
        D3D_FAIL_V(m_device->CreateCommittedResource(
                       &heap_properties,
                       D3D12_HEAP_FLAG_NONE,
                       &buffer_desc,
                       D3D12_RESOURCE_STATE_COMMON,
                       nullptr,
                       IID_PPV_ARGS(&p_out_buffer)),
                   nullptr);
    }

    // Describe the data we want to copy into the default buffer.
    D3D12_SUBRESOURCE_DATA sub_resource_data = {};
    sub_resource_data.pData = p_init_data;
    sub_resource_data.RowPitch = p_byte_size;
    sub_resource_data.SlicePitch = sub_resource_data.RowPitch;

    auto cmd = m_copyContext.Allocate(p_byte_size);
    UpdateSubresources<1>(cmd.commandList.Get(), p_out_buffer, cmd.uploadBuffer.buffer.Get(), 0, 0, 1, &sub_resource_data);
    m_copyContext.Submit(cmd);
    return p_out_buffer;
};

auto D3d12GraphicsManager::CreateBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuBuffer>> {
    auto ret = std::make_shared<D3d12Buffer>(p_desc);

    const uint32_t size_in_byte = p_desc.elementCount * p_desc.elementSize;
    ret->buffer = UploadBuffer(size_in_byte, p_desc.initialData, nullptr);
    if (ret->buffer == nullptr) {
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE);
    }

    return ret;
}

auto D3d12GraphicsManager::CreateMeshImpl(const GpuMeshDesc& p_desc,
                                          uint32_t p_count,
                                          const GpuBufferDesc* p_vb_descs,
                                          const GpuBufferDesc* p_ib_desc) -> Result<std::shared_ptr<GpuMesh>> {
    auto ret = std::make_shared<D3d12MeshBuffers>(p_desc);
    for (uint32_t index = 0; index < p_count; ++index) {
        const auto& vb_desc = p_vb_descs[index];
        if (vb_desc.elementCount == 0) {
            ret->vbvs[index] = { 0, 0, 0 };
            continue;
        }

        auto res = CreateBuffer(p_vb_descs[index]);
        if (!res) {
            return HBN_ERROR(res.error());
        }

        ret->vertexBuffers[index] = *res;
        ret->vbvs[index] = {
            .BufferLocation = ((ID3D12Resource*)(ret->vertexBuffers[index]->GetHandle()))->GetGPUVirtualAddress(),
            .SizeInBytes = vb_desc.elementCount * vb_desc.elementSize,
            .StrideInBytes = vb_desc.elementSize,
        };
    }

    if (p_ib_desc) {
        auto res = CreateBuffer(*p_ib_desc);
        if (!res) {
            return HBN_ERROR(res.error());
        }
        ret->indexBuffer = *res;
        ret->ibv = {
            .BufferLocation = ((ID3D12Resource*)(ret->indexBuffer->GetHandle()))->GetGPUVirtualAddress(),
            .SizeInBytes = p_ib_desc->elementCount * p_ib_desc->elementSize,
            .Format = DXGI_FORMAT_R32_UINT,
        };
    }

    return ret;
}

void D3d12GraphicsManager::SetMesh(const GpuMesh* p_mesh) {
    auto mesh = reinterpret_cast<const D3d12MeshBuffers*>(p_mesh);

    m_graphicsCommandList->IASetVertexBuffers(0, MESH_MAX_VERTEX_BUFFER_COUNT, mesh->vbvs);
    if (mesh->indexBuffer) {
        m_graphicsCommandList->IASetIndexBuffer(&mesh->ibv);
    }
}

void D3d12GraphicsManager::UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) {
    auto buffer = reinterpret_cast<D3d12Buffer*>(p_buffer);
    const uint32_t size_in_byte = p_desc.elementCount * p_desc.elementSize;
    UploadBuffer(size_in_byte, p_desc.initialData, buffer->buffer.Get());
}

void D3d12GraphicsManager::DrawElements(uint32_t p_count, uint32_t p_offset) {
    m_graphicsCommandList->DrawIndexedInstanced(p_count, 1, p_offset, 0, 0);
}

void D3d12GraphicsManager::DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) {
    m_graphicsCommandList->DrawIndexedInstanced(p_count, p_instance_count, p_offset, 0, 0);
}

void D3d12GraphicsManager::DrawArrays(uint32_t p_count, uint32_t p_offset) {
    m_graphicsCommandList->DrawInstanced(p_count, 1, p_offset, 0);
}

void D3d12GraphicsManager::DrawArraysInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) {
    m_graphicsCommandList->DrawInstanced(p_count, p_instance_count, p_offset, 0);
}

void D3d12GraphicsManager::Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) {
    m_graphicsCommandList->Dispatch(p_num_groups_x, p_num_groups_y, p_num_groups_z);
}

void D3d12GraphicsManager::BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) {
    unused(p_slot);
    unused(p_texture);
}

void D3d12GraphicsManager::UnbindUnorderedAccessView(uint32_t p_slot) {
    unused(p_slot);
}

void D3d12GraphicsManager::BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) {
    unused(p_slot);
    auto uav = reinterpret_cast<const D3d12StructuredBuffer*>(p_buffer);
    if (DEV_VERIFY(uav)) {
    }
}

void D3d12GraphicsManager::UnbindStructuredBuffer(int p_slot) {
    unused(p_slot);
}

void D3d12GraphicsManager::BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) {
    unused(p_slot);
    unused(p_buffer);
}

void D3d12GraphicsManager::UnbindStructuredBufferSRV(int p_slot) {
    unused(p_slot);
}

auto D3d12GraphicsManager::CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> {
    const uint32_t size_in_byte = p_desc.elementCount * p_desc.elementSize;
    CD3DX12_HEAP_PROPERTIES heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(size_in_byte);
    ComPtr<ID3D12Resource> buffer;
    D3D_FAIL(m_device->CreateCommittedResource(
                 &heap_properties,
                 D3D12_HEAP_FLAG_NONE,
                 &buffer_desc,
                 D3D12_RESOURCE_STATE_GENERIC_READ,
                 nullptr, IID_PPV_ARGS(&buffer)),
             "Failed to create CommittedResource");

    auto result = std::make_shared<D3d12ConstantBuffer>(p_desc);
    result->buffer = buffer;

    D3D_FAIL(result->buffer->Map(0, nullptr, reinterpret_cast<void**>(&result->mappedData)),
             "Failed to Map buffer");

    return result;
}

auto D3d12GraphicsManager::CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> {
    DEV_ASSERT(!p_desc.initialData && "TODO: initial data");

    CD3DX12_HEAP_PROPERTIES heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC buffer_desc{};
    buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Width = p_desc.elementCount * p_desc.elementSize;
    buffer_desc.Height = 1;
    buffer_desc.DepthOrArraySize = 1;
    buffer_desc.MipLevels = 1;
    buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
    buffer_desc.SampleDesc.Count = 1;
    buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ComPtr<ID3D12Resource> buffer;
    D3D_FAIL(m_device->CreateCommittedResource(
                 &heap_properties,
                 D3D12_HEAP_FLAG_NONE,
                 &buffer_desc,
                 D3D12_RESOURCE_STATE_COMMON,
                 nullptr, IID_PPV_ARGS(&buffer)),
             "Failed to create buffer (StructuredBuffer)");

    // @TODO: set debug name
    LOG_WARN("TODO: set buffer name");
    // SetDebugName(buffer.Get(), "");
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;  // Structured buffer doesn't have a format
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = p_desc.elementCount;
    uav_desc.Buffer.StructureByteStride = p_desc.elementSize;

    auto handle = m_srvDescHeap.AllocHandle();
    m_device->CreateUnorderedAccessView(buffer.Get(), nullptr, &uav_desc, handle.cpuHandle);

    auto result = std::make_shared<D3d12StructuredBuffer>(p_desc);
    result->buffer = buffer;
    result->handle = handle;
    return result;
}

void D3d12GraphicsManager::UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) {
    if (p_size) {
        auto cb = reinterpret_cast<const D3d12ConstantBuffer*>(p_buffer);
        memcpy(cb->mappedData, p_data, p_size);
    }
}

void D3d12GraphicsManager::BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) {
    auto buffer = reinterpret_cast<const D3d12ConstantBuffer*>(p_buffer);
    DEV_ASSERT(p_size + p_offset <= buffer->capacity);

    D3D12_GPU_VIRTUAL_ADDRESS batch_address = buffer->buffer->GetGPUVirtualAddress();

    batch_address += p_offset;
    m_graphicsCommandList->SetGraphicsRootConstantBufferView(buffer->desc.slot, batch_address);
    m_graphicsCommandList->SetComputeRootConstantBufferView(buffer->desc.slot, batch_address);
}

std::shared_ptr<GpuTexture> D3d12GraphicsManager::CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
    unused(p_sampler_desc);

    auto initial_data = reinterpret_cast<const uint8_t*>(p_texture_desc.initialData);

    bool gen_mip_map = false;
    PixelFormat format = p_texture_desc.format;
    DXGI_FORMAT texture_format = d3d::Convert(format);
    DXGI_FORMAT srv_format = d3d::Convert(format);
    D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COPY_DEST;

    if (format == PixelFormat::D32_FLOAT) {
        texture_format = DXGI_FORMAT_R32_TYPELESS;
        srv_format = DXGI_FORMAT_R32_FLOAT;
    } else if (format == PixelFormat::D24_UNORM_S8_UINT) {
        texture_format = DXGI_FORMAT_R24G8_TYPELESS;
    } else if (format == PixelFormat::R24G8_TYPELESS) {
        srv_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    }
    switch (p_texture_desc.type) {
        case AttachmentType::NONE:
            initial_state = D3D12_RESOURCE_STATE_COPY_DEST;
            break;
        default:
            initial_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            break;
    }

    D3D12_HEAP_PROPERTIES props{};
    props.Type = D3D12_HEAP_TYPE_DEFAULT;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    auto ConvertBindFlags = [](BindFlags p_bind_flags) {
        [[maybe_unused]] constexpr BindFlags supported_flags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_DEPTH_STENCIL | BIND_UNORDERED_ACCESS;
        DEV_ASSERT((p_bind_flags & (~supported_flags)) == 0);

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        // if (p_bind_flags & BIND_SHADER_RESOURCE) {
        // }
        if (p_bind_flags & BIND_RENDER_TARGET) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (p_bind_flags & BIND_DEPTH_STENCIL) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        if (p_bind_flags & BIND_UNORDERED_ACCESS) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        return flags;
    };

    D3D12_RESOURCE_DESC texture_desc{};
    texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_desc.Alignment = 0;
    texture_desc.Width = p_texture_desc.width;
    texture_desc.Height = p_texture_desc.height;
    texture_desc.DepthOrArraySize = static_cast<UINT16>(p_texture_desc.arraySize);
    texture_desc.MipLevels = static_cast<UINT16>(gen_mip_map ? 0 : p_texture_desc.mipLevels);
    texture_desc.Format = texture_format;
    texture_desc.SampleDesc = { 1, 0 };
    texture_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texture_desc.Flags = ConvertBindFlags(p_texture_desc.bindFlags);

    ID3D12Resource* texture_ptr = nullptr;
    D3D_FAIL_V(m_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &texture_desc, initial_state, NULL, IID_PPV_ARGS(&texture_ptr)), nullptr);

    if (initial_data) {
        // Create a temporary upload resource to move the data in
        uint32_t byte_width = 1;
        switch (p_texture_desc.format) {
            case PixelFormat::R32G32B32_FLOAT:
            case PixelFormat::R32G32B32A32_FLOAT:
                byte_width = 4;
                break;
            default:
                break;
        }
        const uint32_t upload_pitch = byte_width * math::Align(4 * static_cast<int>(p_texture_desc.width), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        const uint32_t upload_size = p_texture_desc.height * upload_pitch;
        texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        texture_desc.Alignment = 0;
        texture_desc.Width = upload_size;
        texture_desc.Height = 1;
        texture_desc.DepthOrArraySize = 1;
        texture_desc.MipLevels = 1;
        texture_desc.Format = DXGI_FORMAT_UNKNOWN;
        texture_desc.SampleDesc = { 1, 0 };
        texture_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        texture_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        props.Type = D3D12_HEAP_TYPE_UPLOAD;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        // @TODO: refactor
        ComPtr<ID3D12Resource> upload_buffer;
        D3D_FAIL_V(m_device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &texture_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&upload_buffer)), nullptr);

        // Write pixels into the upload resource
        void* mapped = NULL;
        D3D12_RANGE range = { 0, upload_size };

        upload_buffer->Map(0, &range, &mapped);
        const uint32_t byte_per_row = 4 * byte_width * p_texture_desc.width;
        for (uint32_t y = 0; y < p_texture_desc.height; y++) {
            memcpy((void*)((uintptr_t)mapped + y * upload_pitch), initial_data + y * byte_per_row, byte_per_row);
        }
        upload_buffer->Unmap(0, &range);

        // Copy the upload resource content into the real resource
        D3D12_TEXTURE_COPY_LOCATION source_location = {};
        source_location.pResource = upload_buffer.Get();
        source_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source_location.PlacedFootprint.Footprint.Format = d3d::Convert(p_texture_desc.format);
        source_location.PlacedFootprint.Footprint.Width = p_texture_desc.width;
        source_location.PlacedFootprint.Footprint.Height = p_texture_desc.height;
        source_location.PlacedFootprint.Footprint.Depth = 1;
        source_location.PlacedFootprint.Footprint.RowPitch = upload_pitch;

        D3D12_TEXTURE_COPY_LOCATION dest_location = {};
        dest_location.pResource = texture_ptr;
        dest_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dest_location.SubresourceIndex = 0;

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = texture_ptr;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        // Create a temporary command queue to do the copy with
        ComPtr<ID3D12Fence> fence;
        D3D_FAIL_V(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), nullptr);

        HANDLE event = CreateEvent(0, 0, 0, 0);
        if (event == NULL) {
            return nullptr;
        }

        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queue_desc.NodeMask = 1;

        ComPtr<ID3D12CommandQueue> copy_queue;
        D3D_FAIL_V(m_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&copy_queue)), nullptr);

        ComPtr<ID3D12CommandAllocator> copy_alloc;
        D3D_FAIL_V(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&copy_alloc)), nullptr);

        ComPtr<ID3D12GraphicsCommandList> command_list;
        D3D_FAIL_V(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, copy_alloc.Get(), NULL, IID_PPV_ARGS(&command_list)), nullptr);

        command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, NULL);
        command_list->ResourceBarrier(1, &barrier);
        command_list->Close();

        // Execute the copy
        ID3D12CommandList* command_lists[] = { command_list.Get() };
        copy_queue->ExecuteCommandLists(array_length(command_lists), command_lists);
        copy_queue->Signal(fence.Get(), 1);

        // Wait for everything to complete
        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, INFINITE);

        // Tear down our temporary command queue and release the upload resource
        CloseHandle(event);
    }

    auto gpu_texture = std::make_shared<D3d12GpuTexture>(p_texture_desc);
    // Create a shader resource view for the texture
    if (p_texture_desc.bindFlags & BIND_SHADER_RESOURCE) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = srv_format;
        DescriptorResourceType resource_type{};
        switch (p_texture_desc.dimension) {
            case Dimension::TEXTURE_2D:
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srv_desc.Texture2D.MipLevels = texture_desc.MipLevels;
                srv_desc.Texture2D.MostDetailedMip = 0;

                resource_type = DescriptorResourceType::Texture2D;
                break;
            case Dimension::TEXTURE_CUBE:
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srv_desc.TextureCube.MipLevels = texture_desc.MipLevels;
                srv_desc.TextureCube.MostDetailedMip = 0;
                break;
            case Dimension::TEXTURE_CUBE_ARRAY:
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                srv_desc.TextureCubeArray.MipLevels = texture_desc.MipLevels;
                srv_desc.TextureCubeArray.MostDetailedMip = 0;
                srv_desc.TextureCubeArray.First2DArrayFace = 0;
                srv_desc.TextureCubeArray.NumCubes = p_texture_desc.arraySize / 6;

                resource_type = DescriptorResourceType::TextureCubeArray;
                break;
            default:
                CRASH_NOW();
                break;
        }

        auto srv_handle = m_srvDescHeap.AllocBindlessHandle(resource_type);
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        m_device->CreateShaderResourceView(texture_ptr, &srv_desc, srv_handle.cpuHandle);
        gpu_texture->srvHandle = srv_handle;
    }

    if (p_texture_desc.bindFlags & BIND_UNORDERED_ACCESS) {
        LOG_ERROR("@TODO: fix hard code");
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
        uav_desc.Format = texture_format;
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uav_desc.Texture2D.MipSlice = 0;
        auto uav_handle = m_srvDescHeap.AllocBindlessHandle(DescriptorResourceType::RWTexture2D);
        m_device->CreateUnorderedAccessView(texture_ptr, nullptr, &uav_desc, uav_handle.cpuHandle);
        gpu_texture->uavHandle = uav_handle;
    }

    gpu_texture->texture = ComPtr<ID3D12Resource>(texture_ptr);
    SetDebugName(texture_ptr, RenderTargetResourceNameToString(p_texture_desc.name));
    return gpu_texture;
}

void D3d12GraphicsManager::BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) {
    unused(p_dimension);
    unused(p_handle);
    unused(p_slot);
}

void D3d12GraphicsManager::UnbindTexture(Dimension p_dimension, int p_slot) {
    unused(p_dimension);
    unused(p_slot);
}

void D3d12GraphicsManager::GenerateMipmap(const GpuTexture* p_texture) {
    unused(p_texture);
    CRASH_NOW();
}

std::shared_ptr<Framebuffer> D3d12GraphicsManager::CreateFramebuffer(const FramebufferDesc& p_subpass_desc) {
    auto framebuffer = std::make_shared<D3d12Framebuffer>(p_subpass_desc);

    for (const auto& color_attachment : p_subpass_desc.colorAttachments) {
        auto texture = reinterpret_cast<const D3d12GpuTexture*>(color_attachment.get());
        switch (color_attachment->desc.type) {
            case AttachmentType::COLOR_2D: {
                auto handle = m_rtvDescHeap.AllocHandle();
                m_device->CreateRenderTargetView(texture->texture.Get(), nullptr, handle.cpuHandle);
                framebuffer->rtvs.emplace_back(handle.cpuHandle);
            } break;
            case AttachmentType::COLOR_CUBE: {
                for (uint32_t face = 0; face < color_attachment->desc.arraySize; ++face) {
                    auto handle = m_rtvDescHeap.AllocHandle();
                    D3D12_RENDER_TARGET_VIEW_DESC desc{};
                    desc.Format = d3d::Convert(color_attachment->desc.format);
                    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                    desc.Texture2DArray.MipSlice = 0;
                    desc.Texture2DArray.ArraySize = 1;
                    desc.Texture2DArray.FirstArraySlice = face;

                    m_device->CreateRenderTargetView(texture->texture.Get(), &desc, handle.cpuHandle);
                    framebuffer->rtvs.push_back(handle.cpuHandle);
                }
            } break;

            default:
                CRASH_NOW();
                break;
        }
    }

    if (auto& depth_attachment = framebuffer->desc.depthAttachment; depth_attachment) {
        auto texture = reinterpret_cast<const D3d12GpuTexture*>(depth_attachment.get());
        switch (depth_attachment->desc.type) {
            case AttachmentType::DEPTH_2D: {
                // ComPtr<ID3D11DepthStencilView> dsv;
                // D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
                // desc.Format = DXGI_FORMAT_D32_FLOAT;
                // desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                // desc.Texture2D.MipSlice = 0;

                // D3D_FAIL_V_MSG(m_device->CreateDepthStencilView(texture->texture.Get(), &desc, dsv.GetAddressOf()),
                //                nullptr,
                //                "Failed to create depth stencil view");
                // framebuffer->dsvs.push_back(dsv);
                CRASH_NOW();
            } break;
            case AttachmentType::DEPTH_STENCIL_2D: {
                D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Texture2D.MipSlice = 0;
                auto handle = m_dsvDescHeap.AllocHandle();
                m_device->CreateDepthStencilView(texture->texture.Get(), &dsv_desc, handle.cpuHandle);
                framebuffer->dsvs.emplace_back(handle.cpuHandle);
            } break;
            case AttachmentType::SHADOW_2D: {
                D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
                dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Texture2D.MipSlice = 0;

                auto handle = m_dsvDescHeap.AllocHandle();
                m_device->CreateDepthStencilView(texture->texture.Get(), &dsv_desc, handle.cpuHandle);
                framebuffer->dsvs.emplace_back(handle.cpuHandle);
            } break;
            case AttachmentType::SHADOW_CUBE_ARRAY: {
                for (uint32_t face = 0; face < depth_attachment->desc.arraySize; ++face) {
                    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                    dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
                    dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsv_desc.Texture2DArray.MipSlice = 0;
                    dsv_desc.Texture2DArray.ArraySize = 1;
                    dsv_desc.Texture2DArray.FirstArraySlice = face;

                    auto handle = m_dsvDescHeap.AllocHandle();
                    m_device->CreateDepthStencilView(texture->texture.Get(), &dsv_desc, handle.cpuHandle);
                    framebuffer->dsvs.emplace_back(handle.cpuHandle);
                }
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }

    return framebuffer;
}

auto D3d12GraphicsManager::CreateDevice() -> Result<void> {
#if USING(DEBUG_BUILD)
    if (m_enableValidationLayer) {
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController)))) {
            m_debugController->EnableDebugLayer();
        }
    }
#endif

    D3D_FAIL(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)), "failed to create factory");

    auto get_hardware_adapter = [](IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter) {
        *ppAdapter = nullptr;
        for (UINT adapter_index = 0;; ++adapter_index) {
            IDXGIAdapter1* adapter = nullptr;
            if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapter_index, &adapter)) {
                // No more adapters to enumerate.
                break;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))) {
                *ppAdapter = adapter;
                return;
            }
            adapter->Release();
        }
    };

    ComPtr<IDXGIAdapter1> hardware_adapter;
    get_hardware_adapter(m_factory.Get(), &hardware_adapter);

    if (FAILED(D3D12CreateDevice(hardware_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_device.GetAddressOf())))) {
        ComPtr<IDXGIAdapter> warp_adapter;
        D3D_FAIL(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter)), "failed to enum warp adapter");

        D3D_FAIL(D3D12CreateDevice(warp_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_device.GetAddressOf())),
                 "failed to create d3d device");
    }

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels = { array_length(featureLevels), featureLevels, D3D_FEATURE_LEVEL_12_0 };

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
    D3D_CALL(m_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels)));
    featureLevel = featLevels.MaxSupportedFeatureLevel;
    switch (featureLevel) {
        case D3D_FEATURE_LEVEL_12_0:
            LOG("[DX12] Device Feature Level: 12.0");
            break;
        case D3D_FEATURE_LEVEL_12_1:
            LOG("[DX12] Device Feature Level: 12.1");
            break;
        default:
            CRASH_NOW();
            break;
    }

    return Result<void>();
}

auto D3d12GraphicsManager::InitGraphicsContext() -> Result<void> {
    m_graphicsCommandQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    DEV_ASSERT(m_graphicsCommandQueue);

    [[maybe_unused]] int frame_idx = 0;
    for (auto& it : m_frameContexts) {
        D3d12FrameContext& frame = reinterpret_cast<D3d12FrameContext&>(*it.get());
        D3D_FAIL(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.m_commandAllocator)),
                 "failed to create command allocator");
        D3D12_SET_DEBUG_NAME(frame.m_commandAllocator, std::format("GraphicsCommandAllocator {}", frame_idx++));

        if (m_graphicsCommandList == nullptr) {
            D3D_FAIL(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.m_commandAllocator, nullptr, IID_PPV_ARGS(&m_graphicsCommandList)),
                     "failed to create graphics command list");
        }
    }

    m_graphicsCommandList->Close();
    D3D12_SET_DEBUG_NAME(m_graphicsCommandList.Get(), "GraphicsCommandList");

    D3D_FAIL(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_graphicsQueueFence)),
             "failed to create fence");

    D3D12_SET_DEBUG_NAME(m_graphicsQueueFence.Get(), "GraphicsFence");

    m_graphicsFenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

    return Result<void>();
}

void D3d12GraphicsManager::FinalizeGraphicsContext() {
    FlushGraphicsContext();

    m_graphicsQueueFence.Reset();
    m_lastSignaledFenceValue = 0;

    CloseHandle(m_graphicsFenceEvent);
    m_graphicsFenceEvent = NULL;

    m_graphicsCommandList.Reset();
    m_graphicsCommandQueue.Reset();

    m_frameContexts.clear();
}

void D3d12GraphicsManager::FlushGraphicsContext() {
    for (auto& it : m_frameContexts) {
        D3d12FrameContext& frame = reinterpret_cast<D3d12FrameContext&>(*it.get());
        frame.Wait(m_graphicsFenceEvent, m_graphicsQueueFence.Get());
    }
    m_frameIndex = 0;
}

ID3D12CommandQueue* D3d12GraphicsManager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE p_type) {
    D3D12_COMMAND_QUEUE_DESC desc;
    desc.Type = p_type;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

    ID3D12CommandQueue* queue;

    D3D_CALL(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));

#if USING(USE_D3D_DEBUG_NAME)
    switch (p_type) {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            D3D12_SET_DEBUG_NAME(queue, "GraphicsCommandQueue");
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            D3D12_SET_DEBUG_NAME(queue, "ComputeCommandQueue");
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            D3D12_SET_DEBUG_NAME(queue, "CopyCommandQueue");
            break;
        default:
            CRASH_NOW();
            break;
    }
#endif

    return queue;
}

auto D3d12GraphicsManager::EnableDebugLayer() -> Result<void> {
#if USING(DEBUG_BUILD)
    D3D_FAIL(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController)),
             "failed to get debug interface");

    m_debugController->EnableDebugLayer();

    ComPtr<ID3D12Debug> debug_device;
    m_device->QueryInterface(IID_PPV_ARGS(debug_device.GetAddressOf()));

    ComPtr<ID3D12InfoQueue> info_queue;

    D3D_FAIL(m_device->QueryInterface(IID_PPV_ARGS(&info_queue)),
             "failed to query info queue interface");

    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

    D3D12_MESSAGE_ID ignore_list[] = {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
        // D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES
    };

    D3D12_INFO_QUEUE_FILTER filter = {};
    filter.DenyList.pIDList = ignore_list;
    filter.DenyList.NumIDs = array_length(ignore_list);
    info_queue->AddRetrievalFilterEntries(&filter);
    info_queue->AddStorageFilterEntries(&filter);
#endif
    return Result<void>();
}

auto D3d12GraphicsManager::CreateDescriptorHeaps() -> Result<void> {
    if (auto res = m_rtvDescHeap.Initialize(64, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_device.Get(), false); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = m_dsvDescHeap.Initialize(64, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_device.Get(), false); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = m_srvDescHeap.Initialize(512, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_device.Get(), true); !res) {
        return HBN_ERROR(res.error());
    }

    return Result<void>();
}

auto D3d12GraphicsManager::CreateSwapChain(uint32_t p_width, uint32_t p_height) -> Result<void> {
    auto display_manager = dynamic_cast<Win32DisplayManager*>(DisplayManager::GetSingletonPtr());
    DEV_ASSERT(display_manager);

    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC1 scd{};

    // fill the swap chain description struct
    scd.Width = p_width;
    scd.Height = p_height;
    scd.Format = d3d::Convert(DEFAULT_SURFACE_FORMAT);
    scd.Stereo = FALSE;
    scd.SampleDesc = { 1, 0 };
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // how swap chain is to be used
    scd.BufferCount = NUM_FRAMES_IN_FLIGHT;             // back buffer count
    scd.Scaling = DXGI_SCALING_STRETCH;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    IDXGISwapChain1* pSwapChain = nullptr;

    D3D_FAIL(m_factory->CreateSwapChainForHwnd(
                 m_graphicsCommandQueue.Get(),
                 display_manager->GetHwnd(),
                 &scd,
                 NULL,
                 NULL,
                 &pSwapChain),
             "Failed to create swapchain");

    m_swapChain.Attach(reinterpret_cast<IDXGISwapChain3*>(pSwapChain));

    m_swapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
    m_swapChainWaitObject = m_swapChain->GetFrameLatencyWaitableObject();

    return Result<void>();
}

auto D3d12GraphicsManager::CreateRenderTarget(uint32_t p_width, uint32_t p_height) -> Result<void> {
    for (int32_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        ID3D12Resource* backbuffer = nullptr;
        D3D_CALL(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
        m_device->CreateRenderTargetView(backbuffer, nullptr, m_renderTargetDescriptor[i]);
        std::wstring name = std::wstring(L"Render Target Buffer") + std::to_wstring(i);
        backbuffer->SetName(name.c_str());
        m_renderTargets[i] = backbuffer;
    }

    D3D12_CLEAR_VALUE depthOptimizedClearValue{};
    depthOptimizedClearValue.Format = d3d::Convert(DEFAULT_DEPTH_STENCIL_FORMAT);
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES prop{};
    prop.Type = D3D12_HEAP_TYPE_DEFAULT;
    prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    prop.CreationNodeMask = 1;
    prop.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resource_desc{};
    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Alignment = 0;
    resource_desc.Width = p_width;
    resource_desc.Height = p_height;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = d3d::Convert(DEFAULT_DEPTH_STENCIL_FORMAT);
    resource_desc.SampleDesc = { 1, 0 };
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    HRESULT hr = m_device->CreateCommittedResource(
        &prop, D3D12_HEAP_FLAG_NONE, &resource_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue,
        IID_PPV_ARGS(&m_depthStencilBuffer));

    D3D_FAIL(hr, "Failed to create committed resource");

    m_depthStencilBuffer->SetName(L"Depth Stencil Buffer");
    m_device->CreateDepthStencilView(m_depthStencilBuffer, nullptr, m_depthStencilDescriptor);

    return Result<void>();
}

void D3d12GraphicsManager::InitStaticSamplers() {
    auto FillSamplerDesc = [](uint32_t p_slot, const SamplerDesc& p_desc) {
        CD3DX12_STATIC_SAMPLER_DESC sampler_desc(
            p_slot,
            d3d::Convert(p_desc.minFilter, p_desc.magFilter),
            d3d::Convert(p_desc.addressU),
            d3d::Convert(p_desc.addressV),
            d3d::Convert(p_desc.addressW),
            p_desc.mipLodBias,
            p_desc.maxAnisotropy,
            d3d::Convert(p_desc.comparisonFunc),
            d3d::Convert(p_desc.staticBorderColor),
            p_desc.minLod,
            p_desc.maxLod);

        return sampler_desc;
    };

#define SAMPLER_STATE(REG, NAME, DESC) m_staticSamplers.emplace_back(FillSamplerDesc(REG, DESC));
#include "sampler.hlsl.h"
#undef SAMPLER_STATE
}

auto D3d12GraphicsManager::CreateRootSignature() -> Result<void> {
    // Create a root signature consisting of a descriptor table with a single CBV.

    int descriptor_counter = 0;
    CD3DX12_DESCRIPTOR_RANGE descriptor_table[64];
    auto reg_srv_uav = [&](D3D12_DESCRIPTOR_RANGE_TYPE p_type, uint32_t p_count, uint32_t p_space, uint32_t p_offset) {
        descriptor_table[descriptor_counter++].Init(p_type, p_count, 0, p_space, p_offset);
    };

#define DESCRIPTOR_INIT(ENUM, NUM, PREV, SPACE, TYPE) \
    reg_srv_uav(D3D12_DESCRIPTOR_RANGE_TYPE_##TYPE, ENUM##_MAX_COUNT, SPACE, ENUM##_START);
#define DESCRIPTOR_SRV(ENUM, NUM, PREV, SPACE)      DESCRIPTOR_INIT(ENUM, NUM, PREV, SPACE, SRV)
#define DESCRIPTOR_UAV(ENUM, NUM, PREV, SPACE, ...) DESCRIPTOR_INIT(ENUM, NUM, PREV, SPACE, UAV)
    DESCRIPTOR_TABLE
#undef DESCRIPTOR_SRV
#undef DESCRIPTOR_UAV
#undef DESCRIPTOR_INIT

    auto reg_sbuffer = [&](int p_space, int p_offset) {
        // root_parameters[param_count++].InitAsUnorderedAccessView(p_space);
        descriptor_table[descriptor_counter++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, p_offset, p_space, p_offset);
    };
#define SBUFFER(DATA_TYPE, NAME, REG, REG2) reg_sbuffer(REG, REG2);
    SBUFFER_LIST
#undef SBUFFER

    // TODO: Order from most frequent to least frequent.
    CD3DX12_ROOT_PARAMETER root_parameters[16]{};
    int param_count = 0;

    // @TODO: fix this
    root_parameters[param_count++].InitAsConstantBufferView(0);
    root_parameters[param_count++].InitAsConstantBufferView(1);
    root_parameters[param_count++].InitAsConstantBufferView(2);
    root_parameters[param_count++].InitAsConstantBufferView(3);
    root_parameters[param_count++].InitAsConstantBufferView(4);
    root_parameters[param_count++].InitAsConstantBufferView(5);
    root_parameters[param_count++].InitAsConstantBufferView(6);

    root_parameters[param_count++].InitAsDescriptorTable(descriptor_counter, descriptor_table);

    InitStaticSamplers();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(param_count,
                                                    root_parameters,
                                                    (uint32_t)m_staticSamplers.size(),
                                                    m_staticSamplers.data(),
                                                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    // @TODO: print error
    HRESULT hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr)) {
        char buffer[256]{ 0 };
        StringUtils::Sprintf(buffer, "%.*s", error->GetBufferSize(), error->GetBufferPointer());
        LOG_ERROR("Failed to create root signature, reason {}", buffer);
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "Failed to create root signature");
    }

    D3D_FAIL(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
             "Failed to create root signature");

    return Result<void>();
}

void D3d12GraphicsManager::OnWindowResize(int p_width, int p_height) {
    if (m_swapChain) {
        CleanupRenderTarget();
        D3D_CALL(m_swapChain->ResizeBuffers(0, p_width, p_height,
                                            DXGI_FORMAT_UNKNOWN,
                                            DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));

        [[maybe_unused]] auto err = CreateRenderTarget(p_width, p_height);
    }
}

void D3d12GraphicsManager::SetPipelineStateImpl(PipelineStateName p_name) {
    auto pipeline = reinterpret_cast<D3d12PipelineState*>(m_pipelineStateManager->Find(p_name));
    DEV_ASSERT(pipeline);

    auto primitive_topology = d3d::Convert(pipeline->desc.primitiveTopology);
    m_graphicsCommandList->IASetPrimitiveTopology(primitive_topology);

    m_graphicsCommandList->SetPipelineState(pipeline->pso.Get());
}

void D3d12GraphicsManager::CleanupRenderTarget() {
    FlushGraphicsContext();

    for (uint32_t i = 0; i < NUM_BACK_BUFFERS; ++i) {
        SafeRelease(m_renderTargets[i]);
    }
    SafeRelease(m_depthStencilBuffer);
}

}  // namespace my
