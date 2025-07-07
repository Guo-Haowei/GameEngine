#include "opengl4_graphics_manager.h"

#include <imgui/backends/imgui_impl_opengl3.h>

#include "../opengl_common/opengl_helpers.h"
#include "../opengl_common/opengl_prerequisites.h"
#include "../opengl_common/opengl_resources.h"
#include "engine/drivers/glfw/glfw_display_manager.h"
#include "engine/runtime/application.h"
#include "engine/runtime/imgui_manager.h"

namespace my {

static void APIENTRY DebugCallback(GLenum, GLenum, unsigned int, GLenum, GLsizei, const char*, const void*);

auto OpenGL4GraphicsManager::InitializeInternal() -> Result<void> {
    auto display_manager = dynamic_cast<GlfwDisplayManager*>(m_app->GetDisplayServer());
    DEV_ASSERT(display_manager);
    if (!display_manager) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "display manager is nullptr");
    }
    m_window = display_manager->GetGlfwWindow();

    if (gladLoadGL() == 0) {
        LOG_FATAL("[glad] failed to import gl functions");
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE);
    }

    LOG_VERBOSE("[opengl] renderer: {}", (const char*)glGetString(GL_RENDERER));
    LOG_VERBOSE("[opengl] version: {}", (const char*)glGetString(GL_VERSION));

    if (m_enableValidationLayer) {
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(DebugCallback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            LOG_VERBOSE("[opengl] debug callback enabled");
        }
    }

    CreateGpuResources();

    m_meshes.set_description("GPU-Mesh-Allocator");

    auto imgui = m_app->GetImguiManager();
    if (imgui) {
        imgui->SetRenderCallbacks(
            []() {
                ImGui_ImplOpenGL3_Init();
                ImGui_ImplOpenGL3_CreateDeviceObjects();
            },
            []() {
                ImGui_ImplOpenGL3_Shutdown();
            });
    }

    return Result<void>();
}

void OpenGL4GraphicsManager::Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) {
    glDispatchCompute(p_num_groups_x, p_num_groups_y, p_num_groups_z);
    // @TODO: this probably shouldn't be here
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGL4GraphicsManager::BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) {
    DEV_ASSERT(p_texture);
    auto internal_format = gl::ConvertInternalFormat(p_texture->desc.format);
    glBindImageTexture(p_slot, p_texture->GetHandle32(), 0, GL_TRUE, 0, GL_READ_WRITE, internal_format);
}

void OpenGL4GraphicsManager::UnbindUnorderedAccessView(uint32_t p_slot) {
    glBindImageTexture(p_slot, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R11F_G11F_B10F);
}

void OpenGL4GraphicsManager::BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) {
    auto buffer = reinterpret_cast<const OpenGlStructuredBuffer*>(p_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->handle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, p_slot, buffer->handle);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return;
}

void OpenGL4GraphicsManager::UnbindStructuredBuffer(int p_slot) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, p_slot, 0);
}

auto OpenGL4GraphicsManager::CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> {
    GLuint handle = 0;
    glGenBuffers(1, &handle);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
    glBufferData(GL_SHADER_STORAGE_BUFFER, p_desc.elementCount * p_desc.elementSize, p_desc.initialData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    auto buffer = std::make_shared<OpenGlStructuredBuffer>(p_desc);
    buffer->handle = handle;
    return buffer;
}

void OpenGL4GraphicsManager::UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) {
    auto buffer = reinterpret_cast<const OpenGlStructuredBuffer*>(p_buffer);
    if (DEV_VERIFY(buffer)) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->handle);
        float* ptr = (float*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        DEV_ASSERT(ptr);
        memcpy(ptr + p_desc.offset, p_desc.initialData, p_desc.elementCount * p_desc.elementSize);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}

static void APIENTRY DebugCallback(GLenum p_source,
                                   GLenum p_type,
                                   uint32_t p_id,
                                   GLenum p_severity,
                                   GLsizei,
                                   const char* p_message,
                                   const void*) {
    switch (p_id) {
        case 131185:
        case 131204:
            return;
    }

    const char* source = "GL_DEBUG_SOURCE_OTHER";
    switch (p_source) {
        case GL_DEBUG_SOURCE_API:
            source = "GL_DEBUG_SOURCE_API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source = "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source = "GL_DEBUG_SOURCE_SHADER_COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source = "GL_DEBUG_SOURCE_THIRD_PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source = "GL_DEBUG_SOURCE_APPLICATION";
            break;
        default:
            break;
    }

    const char* type = "GL_DEBUG_TYPE_OTHER";
    switch (p_type) {
        case GL_DEBUG_TYPE_ERROR:
            type = "GL_DEBUG_TYPE_ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type = "GL_DEBUG_TYPE_PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type = "GL_DEBUG_TYPE_PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type = "GL_DEBUG_TYPE_MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            [[fallthrough]];
        case GL_DEBUG_TYPE_POP_GROUP:
            return;
        default:
            break;
    }

    LogLevel level = LOG_LEVEL_NORMAL;
    switch (p_severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            level = LOG_LEVEL_ERROR;
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            level = LOG_LEVEL_WARN;
            break;
        case GL_DEBUG_SEVERITY_LOW:
            return;
        default:
            break;
    }

    my::LogImpl(level, std::format("[opengl] {}\n\t| id: {} | source: {} | type: {}", p_message, p_id, source, type));
}

}  // namespace my
