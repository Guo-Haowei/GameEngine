#include "module_registry.h"

#if USING(PLATFORM_WINDOWS)
#include "modules/d3d11/d3d11_graphics_manager.h"
#include "modules/d3d12/d3d12_graphics_manager.h"
//#include "modules/vk/vulkan_graphics_manager.h"
#include "modules/opengl4/opengl4_graphics_manager.h"
#elif USING(PLATFORM_APPLE)
#include "engine/drivers/metal/metal_graphics_manager.h"
#elif USING(PLATFORM_WASM)
#include "modules/opengles3/opengles3_graphics_manager.h"
#endif

#include "engine/renderer/graphics_dvars.h"

namespace my {

template<class T1, class FALLBACK>
inline T1* CreateModule() {
    if (T1::s_createFunc) {
        return T1::s_createFunc();
    }
    return new FALLBACK;
}

class NullDisplayManager : public DisplayManager {
public:
    void FinalizeImpl() override {}

    bool ShouldClose() override { return true; }

    std::tuple<int, int> GetWindowSize() override {
        return std::make_tuple(0, 0);
    }
    std::tuple<int, int> GetWindowPos() override {
        return std::make_tuple(0, 0);
    }

    void BeginFrame() override {}

protected:
    auto InitializeWindow(const WindowSpecfication&) -> Result<void> override {
        return Result<void>();
    }
    void InitializeKeyMapping() override {};
};

WARNING_PUSH()
WARNING_DISABLE(4100, "-Wunused-parameter")

class NullGraphicsManager : public IGraphicsManager {
public:
    NullGraphicsManager() : IGraphicsManager("NullGraphicsManager") {}

    auto InitializeImpl() -> Result<void> override { return Result<void>(); }
    void FinalizeImpl() override {}

    void Update(Scene& p_scene) override {}

    // resource
    auto CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> override { return nullptr; }
    auto CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> override { return nullptr; }
    void UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) override {}

    void SetRenderTarget(const Framebuffer* p_framebuffer, int p_index = 0, int p_mip_level = 0) override {}
    void UnsetRenderTarget() override {}
    void BeginDrawPass(const Framebuffer* p_framebuffer) override {}
    void EndDrawPass(const Framebuffer* p_framebuffer) override {}

    void Clear(const Framebuffer* p_framebuffer,
               ClearFlags p_flags,
               const float* p_clear_color = DEFAULT_CLEAR_COLOR,
               float p_clear_depth = 1.0f,
               uint8_t p_clear_stencil = 0,
               int p_index = 0) override {}

    void SetViewport(const Viewport& p_viewport) override {}

    auto CreateBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuBuffer>> override { return nullptr; }
    void UpdateBuffer(const GpuBufferDesc& p_desc, GpuBuffer* p_buffer) override {}

    auto CreateMesh(const MeshComponent& p_mesh) -> Result<std::shared_ptr<GpuMesh>> override { return nullptr; }

    auto CreateMeshImpl(const GpuMeshDesc& p_desc,
                        uint32_t p_count,
                        const GpuBufferDesc* p_vb_descs,
                        const GpuBufferDesc* p_ib_desc) -> Result<std::shared_ptr<GpuMesh>> override { return nullptr; }

    void SetMesh(const GpuMesh* p_mesh) override {}

    void DrawElements(uint32_t p_count, uint32_t p_offset = 0) override {}
    void DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset = 0) override {}
    void DrawArrays(uint32_t p_count, uint32_t p_offset = 0) override {}
    void DrawArraysInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset = 0) override {}

    void Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) override {}
    void BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) override {}
    void UnbindUnorderedAccessView(uint32_t p_slot) override {}

    void SetPipelineState(PipelineStateName p_name) override {}

    void SetStencilRef(uint32_t p_ref) override {}
    void SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) override {}

    void BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) override {}
    void UnbindStructuredBuffer(int p_slot) override {}
    void BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) override {}
    void UnbindStructuredBufferSRV(int p_slot) override {}

    void UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) override {}

    void BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) override {}

    std::shared_ptr<Framebuffer> CreateFramebuffer(const FramebufferDesc& p_desc) override { return nullptr; }

    std::shared_ptr<GpuTexture> CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) override { return nullptr; }
    std::shared_ptr<GpuTexture> FindTexture(RenderTargetResourceName p_name) const override { return nullptr; }
    void BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) override {}
    void UnbindTexture(Dimension p_dimension, int p_slot) override {}

    void GenerateMipmap(const GpuTexture* p_texture) override {}

    void RequestTexture(ImageAsset* p_image) override {}

    uint64_t GetFinalImage() const override { return 0; }

    Backend GetBackend() const override { return Backend::EMPTY; }

    RenderGraphName GetActiveRenderGraphName() const override { return static_cast<RenderGraphName>(0); }
    bool SetActiveRenderGraph(RenderGraphName p_name) override { return true; }
    renderer::RenderGraph* GetActiveRenderGraph() override { return nullptr; }

    FrameContext& GetCurrentFrame() override {
        FrameContext* context = nullptr;
        return *context;
    }

    void DrawQuad() override {}
    void DrawQuadInstanced(uint32_t p_instance_count) override {}
    void DrawSkybox() override {}

    void EventReceived(std::shared_ptr<IEvent> p_event) override {}

protected:
    std::shared_ptr<GpuTexture> CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) override { return nullptr; }

    void Render() override {}
    void Present() override {}

    void BeginFrame() override {}
    void EndFrame() override {}
    void MoveToNextFrame() override {}
    std::shared_ptr<FrameContext> CreateFrameContext() override { return nullptr; }

    void OnSceneChange(const Scene& p_scene) override {}
    void OnWindowResize(int p_width, int p_height) override {}
    void SetPipelineStateImpl(PipelineStateName p_name) override {}
    void UpdateEmitters(const Scene& p_scene) override {}
};

WARNING_POP()

class NullPhysicsManager : public IPhysicsManager {
public:
    NullPhysicsManager() : IPhysicsManager("NullPhysicsManager") {}
    virtual ~NullPhysicsManager() = default;

    virtual auto InitializeImpl() -> Result<void> { return Result<void>(); }
    virtual void FinalizeImpl() {}

    virtual void Update(Scene&) {}

    virtual void OnSimBegin(Scene&) {}
    virtual void OnSimEnd(Scene&) {}
};

DisplayManager* CreateDisplayManager() {
    return CreateModule<DisplayManager, NullDisplayManager>();
}

IPhysicsManager* CreatePhysicsManager() {
    return CreateModule<IPhysicsManager, NullPhysicsManager>();
}

static IGraphicsManager* SelectGraphicsManager(const std::string& p_backend) {
    if (p_backend == "d3d11") {
#if USING(PLATFORM_WINDOWS)
        return new D3d11GraphicsManager();
#else
        return nullptr;
#endif
    }

    if (p_backend == "d3d12") {
#if USING(PLATFORM_WINDOWS)
        return new D3d12GraphicsManager;
#else
        return nullptr;
#endif
    }

    if (p_backend == "opengl") {
#if USING(PLATFORM_WINDOWS)
        return new OpenGL4GraphicsManager;
#elif USING(PLATFORM_WASM)
        return new OpenGLES3GraphicsManager;
#else
        return nullptr;
#endif
    }

    if (p_backend == "vulkan") {
        return nullptr;
    }

    if (p_backend == "metal") {
        return nullptr;
    }

    return nullptr;
}


IGraphicsManager* CreateGraphicsManager() {
    if (IGraphicsManager::s_createFunc) {
        return IGraphicsManager::s_createFunc();
    }

    const std::string& backend = DVAR_GET_STRING(gfx_backend);
    IGraphicsManager* manager = SelectGraphicsManager(backend);

    if (!manager) {
        manager = new NullGraphicsManager;

        LOG_ERROR("backend '{}' not supported, fallback to NullGraphicsManager", backend);
    }

    return manager;
}

}  // namespace my
