#include "opengl_graphics_manager.h"

#include "engine/core/debugger/profiler.h"
#include "engine/core/framework/application.h"
#include "engine/core/framework/asset_manager.h"
#include "engine/core/framework/imgui_manager.h"
#include "engine/core/math/geometry.h"
#include "engine/drivers/glfw/glfw_display_manager.h"
#include "engine/drivers/opengl/opengl_pipeline_state_manager.h"
#include "engine/drivers/opengl/opengl_prerequisites.h"
#include "engine/drivers/opengl/opengl_resources.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/render_graph/render_graph_defines.h"
#include "engine/scene/scene.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "vsinput.glsl.h"

// @NOTE: include GLFW after opengl
#include <GLFW/glfw3.h>

// @TODO: remove
#include "engine/renderer/ltc_matrix.h"

// @TODO: refactor
using namespace my;
using my::renderer::RenderGraph;
using my::renderer::RenderPass;

template<typename T>
static void BufferStorage(GLuint p_buffer, const std::vector<T>& p_data, bool p_is_dynamic) {
    glNamedBufferStorage(p_buffer, sizeof(T) * p_data.size(), p_data.data(), p_is_dynamic ? GL_DYNAMIC_STORAGE_BIT : 0);
}

static inline void BindToSlot(GLuint buffer, int slot, int size) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(slot, size, GL_FLOAT, GL_FALSE, size * sizeof(float), 0);
    glEnableVertexAttribArray(slot);
}

static unsigned int LoadMTexture(const float* matrixTable) {
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_FLOAT, matrixTable);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

static uint64_t MakeTextureResident(uint32_t texture) {
    uint64_t ret = glGetTextureHandleARB(texture);
    glMakeTextureHandleResidentARB(ret);
    return ret;
}

//-----------------------------------------------------------------------------------------------------------------

namespace my {

static void APIENTRY DebugCallback(GLenum, GLenum, unsigned int, GLenum, GLsizei, const char*, const void*);

OpenGlGraphicsManager::OpenGlGraphicsManager() : GraphicsManager("OpenGlGraphicsManager", Backend::OPENGL, 1) {
    m_pipelineStateManager = std::make_shared<OpenGlPipelineStateManager>();
}

auto OpenGlGraphicsManager::InitializeInternal() -> Result<void> {
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

void OpenGlGraphicsManager::FinalizeImpl() {
    m_pipelineStateManager->Finalize();
}

void OpenGlGraphicsManager::SetPipelineStateImpl(PipelineStateName p_name) {
    auto pipeline = reinterpret_cast<OpenGlPipelineState*>(m_pipelineStateManager->Find(p_name));

    if (pipeline->desc.rasterizerDesc) {
        const auto cull_mode = pipeline->desc.rasterizerDesc->cullMode;
        if (cull_mode != m_stateCache.cullMode) {
            switch (cull_mode) {
                case my::CullMode::NONE:
                    glDisable(GL_CULL_FACE);
                    break;
                case my::CullMode::FRONT:
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                    break;
                case my::CullMode::BACK:
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    break;
                case my::CullMode::FRONT_AND_BACK:
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT_AND_BACK);
                    break;
                default:
                    CRASH_NOW();
                    break;
            }
            m_stateCache.cullMode = cull_mode;
        }

        const bool front_counter_clockwise = pipeline->desc.rasterizerDesc->frontCounterClockwise;
        if (front_counter_clockwise != m_stateCache.frontCounterClockwise) {
            glFrontFace(front_counter_clockwise ? GL_CCW : GL_CW);
            m_stateCache.frontCounterClockwise = front_counter_clockwise;
        }
    }

    if (pipeline->desc.depthStencilDesc) {
        {
            const bool enable_depth_test = pipeline->desc.depthStencilDesc->depthEnabled;
            if (enable_depth_test != m_stateCache.enableDepthTest) {
                if (enable_depth_test) {
                    glEnable(GL_DEPTH_TEST);
                } else {
                    glDisable(GL_DEPTH_TEST);
                }
                m_stateCache.enableDepthTest = enable_depth_test;
            }

            if (enable_depth_test) {
                const auto func = pipeline->desc.depthStencilDesc->depthFunc;
                if (func != m_stateCache.depthFunc) {
                    glDepthFunc(gl::Convert(func));
                    m_stateCache.depthFunc = func;
                }
            }
        }
        {
            const bool enable_stencil_test = pipeline->desc.depthStencilDesc->stencilEnabled;
            if (enable_stencil_test != m_stateCache.enableStencilTest) {
                if (enable_stencil_test) {
                    glEnable(GL_STENCIL_TEST);
                } else {
                    glDisable(GL_STENCIL_TEST);
                }
                m_stateCache.enableStencilTest = enable_stencil_test;
            }

            if (enable_stencil_test) {
                const auto& face = pipeline->desc.depthStencilDesc->frontFace;
                const auto stencil_func = gl::Convert(face.stencilFunc);
                const auto fail_op = gl::Convert(face.stencilFailOp);
                const auto zfail_op = gl::Convert(face.stencilDepthFailOp);
                const auto zpass_op = gl::Convert(face.stencilPassOp);
                glStencilOp(fail_op, zfail_op, zpass_op);
                glStencilFunc(stencil_func, 0, 0xFF);
                m_stateCache.stencilFunc = face.stencilFunc;
            }
        }
    }
    if (auto blend_desc = pipeline->desc.blendDesc; blend_desc) {
        SetBlendState(*blend_desc, nullptr, 0);
    }

    m_stateCache.topology = gl::Convert(pipeline->desc.primitiveTopology);

    glUseProgram(pipeline->programId);
}

void OpenGlGraphicsManager::Clear(const DrawPass* p_draw_pass, ClearFlags p_flags, const float* p_clear_color, int p_index) {
    unused(p_draw_pass);
    unused(p_index);

    if (p_flags == CLEAR_NONE) {
        return;
    }

    uint32_t flags = 0;
    if (p_flags & CLEAR_COLOR_BIT) {
        // @TODO: cache clear color
        if (p_clear_color) {
            glClearColor(p_clear_color[0], p_clear_color[1], p_clear_color[2], p_clear_color[3]);
        }
        flags |= GL_COLOR_BUFFER_BIT;
    }
    if (p_flags & CLEAR_DEPTH_BIT) {
        flags |= GL_DEPTH_BUFFER_BIT;
    }
    if (p_flags & CLEAR_STENCIL_BIT) {
        flags |= GL_STENCIL_BUFFER_BIT;
    }

    glClear(flags);
}

void OpenGlGraphicsManager::SetViewport(const Viewport& p_viewport) {
    if (p_viewport.topLeftY) {
        LOG_FATAL("TODO: adjust to bottom left y");
    }

    glViewport(p_viewport.topLeftX,
               p_viewport.topLeftY,
               p_viewport.width,
               p_viewport.height);
}

LineBuffers* OpenGlGraphicsManager::CreateLine(const std::vector<Point>& p_points) {
    // @TODO: fix
    OpenGlLineBuffers* buffers = new OpenGlLineBuffers;
    glGenVertexArrays(1, &buffers->vao);
    glGenBuffers(1, &buffers->vbo);

    auto vao = buffers->vao;
    auto vbo = buffers->vbo;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (GLvoid*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point), (GLvoid*)(3 * sizeof(GLfloat)));

    BufferStorage(vbo, p_points, true);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    buffers->capacity = (uint32_t)p_points.size();
    return buffers;
}

void OpenGlGraphicsManager::SetLine(const LineBuffers* p_buffer) {
    // HACK:
    glLineWidth(4.0f);
    auto buffer = reinterpret_cast<const OpenGlLineBuffers*>(p_buffer);
    glBindVertexArray(buffer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);
}

void OpenGlGraphicsManager::UpdateLine(LineBuffers* p_buffer, const std::vector<Point>& p_points) {
    auto buffer = reinterpret_cast<OpenGlLineBuffers*>(p_buffer);
    {
        const uint32_t size_in_byte = sizeof(Point) * (uint32_t)p_points.size();
        glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size_in_byte, p_points.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

// @TODO: refactor
enum BUFFER_TYPE {
    ARRAY_BUFFER = GL_ARRAY_BUFFER,
    INDEX_BUFFER = GL_ELEMENT_ARRAY_BUFFER,
};

BUFFER_TYPE Convert(GpuBufferType p_type) {
    switch (p_type) {
        case GpuBufferType::VERTEX:
            return ARRAY_BUFFER;
        case GpuBufferType::INDEX:
            return INDEX_BUFFER;
        case GpuBufferType::CONSTANT:
        case GpuBufferType::STRUCTURED:
        default:
            CRASH_NOW();
            return ARRAY_BUFFER;
    }
};

auto OpenGlGraphicsManager::CreateBuffer(const GpuBufferDesc& p_desc) -> std::shared_ptr<GpuBuffer> {
    auto type = Convert(p_desc.type);

    // @TODO: fix this
    // CPU_ACCESS_BIT?
    const bool is_dynamic = false;

    GLuint handle = 0;
    glGenBuffers(1, &handle);
    glBindBuffer(type, handle);
    glNamedBufferStorage(handle,
                         p_desc.elementCount * p_desc.elementSize,
                         p_desc.initialData,
                         is_dynamic ? GL_DYNAMIC_STORAGE_BIT : 0);

    auto buffer = std::make_shared<OpenGlBuffer>(p_desc);
    buffer->handle = handle;
    buffer->type = type;
    return buffer;
}

auto OpenGlGraphicsManager::CreateMeshImpl(const GpuMeshDesc& p_desc,
                                           uint32_t p_count,
                                           const GpuBufferDesc* p_vb_descs,
                                           const GpuBufferDesc* p_ib_desc) -> Result<std::shared_ptr<GpuMesh>> {

    auto ret = std::make_shared<OpenGlMeshBuffers>(p_desc);

    // create VAO
    glGenVertexArrays(1, &ret->vao);

    ret->indexBuffer = CreateBuffer(*p_ib_desc);
    DEV_ASSERT(ret->indexBuffer);
    if (!ret->indexBuffer) {
        return nullptr;
    }

    GLuint ebo = (uint32_t)ret->indexBuffer->handle;
    DEV_ASSERT(ret->vao && ebo);

    glBindVertexArray(ret->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    for (uint32_t index = 0; index < p_count; ++index) {
        if (p_vb_descs[index].elementCount) {
            ret->vertexBuffers[index] = CreateBuffer(p_vb_descs[index]);

            const GLuint vbo = (GLuint)ret->vertexBuffers[index]->handle;
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            const uint32_t stride_in_byte = ret->desc.vertexLayout[index].strideInByte;
            glVertexAttribPointer(index,
                                  stride_in_byte / sizeof(float),
                                  GL_FLOAT,
                                  GL_FALSE,
                                  stride_in_byte,
                                  0);
            glEnableVertexAttribArray(index);
        }
    }

    glBindVertexArray(0);
    return ret;
}

void OpenGlGraphicsManager::SetMesh(const GpuMesh* p_mesh) {
    auto mesh = reinterpret_cast<const OpenGlMeshBuffers*>(p_mesh);
    glBindVertexArray(mesh->vao);
}

void OpenGlGraphicsManager::UpdateMesh(GpuMesh* p_mesh, const std::vector<Vector3f>& p_positions, const std::vector<Vector3f>& p_normals) {
    // @TODO: update buffer
    if (auto mesh = dynamic_cast<OpenGlMeshBuffers*>(p_mesh); DEV_VERIFY(mesh)) {
        CRASH_NOW();
        {
            const uint32_t size_in_byte = sizeof(Vector3f) * (uint32_t)p_positions.size();
            // glBindBuffer(GL_ARRAY_BUFFER, mesh->vbos[0]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, size_in_byte, p_positions.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        if constexpr (1) {
            const uint32_t size_in_byte = sizeof(Vector3f) * (uint32_t)p_normals.size();
            // glBindBuffer(GL_ARRAY_BUFFER, mesh->vbos[1]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, size_in_byte, p_normals.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}

void OpenGlGraphicsManager::DrawElements(uint32_t p_count, uint32_t p_offset) {
    glDrawElements(m_stateCache.topology, p_count, GL_UNSIGNED_INT, (void*)(p_offset * sizeof(uint32_t)));
}

void OpenGlGraphicsManager::DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) {
    glDrawElementsInstanced(m_stateCache.topology, p_count, GL_UNSIGNED_INT, (void*)(p_offset * sizeof(uint32_t)), p_instance_count);
}

void OpenGlGraphicsManager::DrawArrays(uint32_t p_count, uint32_t p_offset) {
    glDrawArrays(m_stateCache.topology, p_offset, p_count);
}

void OpenGlGraphicsManager::Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) {
    glDispatchCompute(p_num_groups_x, p_num_groups_y, p_num_groups_z);
    // @TODO: this probably shouldn't be here
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGlGraphicsManager::BindUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) {
    DEV_ASSERT(p_texture);
    auto internal_format = gl::ConvertInternalFormat(p_texture->desc.format);
    glBindImageTexture(p_slot, p_texture->GetHandle32(), 0, GL_TRUE, 0, GL_READ_WRITE, internal_format);
}

void OpenGlGraphicsManager::UnbindUnorderedAccessView(uint32_t p_slot) {
    glBindImageTexture(p_slot, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R11F_G11F_B10F);
}

void OpenGlGraphicsManager::BindStructuredBuffer(int p_slot, const GpuStructuredBuffer* p_buffer) {
    auto buffer = reinterpret_cast<const OpenGlStructuredBuffer*>(p_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->handle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, p_slot, buffer->handle);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return;
}

// @TODO: refactor
void OpenGlGraphicsManager::UnbindStructuredBuffer(int p_slot) {
    unused(p_slot);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, p_slot, 0);
}

void OpenGlGraphicsManager::BindStructuredBufferSRV(int p_slot, const GpuStructuredBuffer* p_buffer) {
    BindStructuredBuffer(p_slot, p_buffer);
}

void OpenGlGraphicsManager::UnbindStructuredBufferSRV(int p_slot) {
    UnbindStructuredBuffer(p_slot);
}

auto OpenGlGraphicsManager::CreateConstantBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuConstantBuffer>> {
    GLuint handle = 0;

    glGenBuffers(1, &handle);
    if (handle == 0) {
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "failed to generate buffer");
    }

    glBindBuffer(GL_UNIFORM_BUFFER, handle);
    glBufferData(GL_UNIFORM_BUFFER, p_desc.elementCount * p_desc.elementSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, p_desc.slot, handle);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    auto buffer = std::make_shared<OpenGlUniformBuffer>(p_desc);
    buffer->handle = handle;
    return buffer;
}

auto OpenGlGraphicsManager::CreateStructuredBuffer(const GpuBufferDesc& p_desc) -> Result<std::shared_ptr<GpuStructuredBuffer>> {
    GLuint handle = 0;
    glGenBuffers(1, &handle);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
    glBufferData(GL_SHADER_STORAGE_BUFFER, p_desc.elementCount * p_desc.elementSize, p_desc.initialData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    auto buffer = std::make_shared<OpenGlStructuredBuffer>(p_desc);
    buffer->handle = handle;
    return buffer;
}

void OpenGlGraphicsManager::UpdateBufferData(const GpuBufferDesc& p_desc, const GpuStructuredBuffer* p_buffer) {
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

void OpenGlGraphicsManager::UpdateConstantBuffer(const GpuConstantBuffer* p_buffer, const void* p_data, size_t p_size) {
    auto buffer = reinterpret_cast<const OpenGlUniformBuffer*>(p_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer->handle);
    glBufferData(GL_UNIFORM_BUFFER, p_size, p_data, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGlGraphicsManager::BindConstantBufferRange(const GpuConstantBuffer* p_buffer, uint32_t p_size, uint32_t p_offset) {
    auto buffer = reinterpret_cast<const OpenGlUniformBuffer*>(p_buffer);
    DEV_ASSERT(p_size + p_offset <= buffer->capacity);
    glBindBufferRange(GL_UNIFORM_BUFFER, p_buffer->GetSlot(), buffer->handle, p_offset, p_size);
}

void OpenGlGraphicsManager::BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) {
    if (p_handle == 0) {
        return;
    }

    const GLuint texture_type = gl::ConvertDimension(p_dimension);
    glActiveTexture(GL_TEXTURE0 + p_slot);
    glBindTexture(texture_type, static_cast<GLuint>(p_handle));
}

void OpenGlGraphicsManager::UnbindTexture(Dimension p_dimension, int p_slot) {
    const GLuint texture_type = gl::ConvertDimension(p_dimension);

    glActiveTexture(GL_TEXTURE0 + p_slot);
    glBindTexture(texture_type, 0);
}

void OpenGlGraphicsManager::GenerateMipmap(const GpuTexture* p_texture) {
    auto dimension = gl::ConvertDimension(p_texture->desc.dimension);
    glBindTexture(dimension, p_texture->GetHandle32());
    glGenerateMipmap(dimension);
    glBindTexture(dimension, 0);
}

std::shared_ptr<GpuTexture> OpenGlGraphicsManager::CreateTextureImpl(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
    GLuint texture_id = 0;
    glGenTextures(1, &texture_id);

    GLenum texture_type = gl::ConvertDimension(p_texture_desc.dimension);
    GLenum internal_format = gl::ConvertInternalFormat(p_texture_desc.format);
    GLenum format = gl::ConvertFormat(p_texture_desc.format);
    GLenum data_type = gl::ConvertDataType(p_texture_desc.format);

    glBindTexture(texture_type, texture_id);

    switch (texture_type) {
        case GL_TEXTURE_2D: {
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         internal_format,
                         p_texture_desc.width,
                         p_texture_desc.height,
                         0,
                         format,
                         data_type,
                         p_texture_desc.initialData);
        } break;
        // @TODO: same
        case GL_TEXTURE_CUBE_MAP: {
            for (int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                             0,
                             internal_format,
                             p_texture_desc.width,
                             p_texture_desc.height,
                             0,
                             format,
                             data_type,
                             p_texture_desc.initialData);
            }
        } break;
        case GL_TEXTURE_CUBE_MAP_ARRAY: {
            glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY,
                         0,
                         internal_format,
                         p_texture_desc.width,
                         p_texture_desc.height,
                         p_texture_desc.arraySize,
                         0, format, data_type, p_texture_desc.initialData);
        } break;
        case GL_TEXTURE_3D: {
            glTexStorage3D(GL_TEXTURE_3D,
                           p_texture_desc.mipLevels,
                           internal_format,
                           p_texture_desc.width,
                           p_texture_desc.height,
                           p_texture_desc.depth);
        } break;
        default:
            CRASH_NOW();
            break;
    }

    gl::SetSampler(texture_type, p_sampler_desc);
    if (p_texture_desc.miscFlags & RESOURCE_MISC_GENERATE_MIPS) {
        glGenerateMipmap(texture_type);
    }

    glBindTexture(texture_type, 0);

    GLuint64 resident_id = glGetTextureHandleARB(texture_id);
    glMakeTextureHandleResidentARB(resident_id);

    auto texture = std::make_shared<OpenGlGpuTexture>(p_texture_desc);
    texture->handle = texture_id;
    texture->residentHandle = resident_id;
    return texture;
}

std::shared_ptr<DrawPass> OpenGlGraphicsManager::CreateDrawPass(const DrawPassDesc& p_desc) {
    auto draw_pass = std::make_shared<OpenGlDrawPass>(p_desc);
    GLuint fbo_handle = 0;

    const int num_depth_attachment = p_desc.depthAttachment != nullptr;
    const int num_color_attachment = (int)p_desc.colorAttachments.size();
    if (!num_depth_attachment && !num_color_attachment) {
        return draw_pass;
    }

    glGenFramebuffers(1, &fbo_handle);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_handle);

    if (!num_color_attachment) {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    } else {
        // create color attachments
        std::vector<GLuint> attachments;
        attachments.reserve(num_color_attachment);
        for (int idx = 0; idx < num_color_attachment; ++idx) {
            GLuint attachment = GL_COLOR_ATTACHMENT0 + idx;
            attachments.push_back(attachment);

            const auto& color_attachment = p_desc.colorAttachments[idx];
            uint32_t texture_handle = static_cast<uint32_t>(color_attachment->GetHandle());
            switch (color_attachment->desc.type) {
                case AttachmentType::COLOR_2D: {
                    glFramebufferTexture2D(GL_FRAMEBUFFER,  // target
                                           attachment,      // attachment
                                           GL_TEXTURE_2D,   // texture target
                                           texture_handle,  // texture
                                           0                // level
                    );
                } break;
                case AttachmentType::COLOR_CUBE: {
                } break;
                default:
                    CRASH_NOW();
                    break;
            }
        }

        glDrawBuffers(num_color_attachment, attachments.data());
    }

    if (auto depth_attachment = p_desc.depthAttachment; depth_attachment) {
        uint32_t texture_handle = static_cast<uint32_t>(depth_attachment->GetHandle());
        switch (depth_attachment->desc.type) {
            case AttachmentType::SHADOW_2D:
            case AttachmentType::DEPTH_2D: {
                glFramebufferTexture2D(GL_FRAMEBUFFER,       // target
                                       GL_DEPTH_ATTACHMENT,  // attachment
                                       GL_TEXTURE_2D,        // texture target
                                       texture_handle,       // texture
                                       0);                   // level
            } break;
            case AttachmentType::DEPTH_STENCIL_2D: {
                glFramebufferTexture2D(GL_FRAMEBUFFER,               // target
                                       GL_DEPTH_STENCIL_ATTACHMENT,  // attachment
                                       GL_TEXTURE_2D,                // texture target
                                       texture_handle,               // texture
                                       0);                           // level
            } break;
            case AttachmentType::SHADOW_CUBE_ARRAY: {
                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_handle, 0);
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }

    // DEV_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    draw_pass->handle = fbo_handle;
    return draw_pass;
}

void OpenGlGraphicsManager::SetStencilRef(uint32_t p_ref) {
    glStencilFunc(gl::Convert(m_stateCache.stencilFunc), p_ref, 0xFF);
}

void OpenGlGraphicsManager::SetBlendState(const BlendDesc& p_desc, const float* p_factor, uint32_t p_mask) {
    unused(p_factor);
    unused(p_mask);

    const auto& desc = p_desc.renderTargets[0];
    if (desc.blendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
        return;
    }

    const bool r_mask = desc.colorWriteMask & COLOR_WRITE_ENABLE_RED;
    const bool g_mask = desc.colorWriteMask & COLOR_WRITE_ENABLE_GREEN;
    const bool b_mask = desc.colorWriteMask & COLOR_WRITE_ENABLE_BLUE;
    const bool a_mask = desc.colorWriteMask & COLOR_WRITE_ENABLE_ALPHA;

    glColorMask(r_mask, g_mask, b_mask, a_mask);

    auto src_blend = gl::Convert(desc.blendSrc);
    auto dest_blend = gl::Convert(desc.blendDest);
    auto func = gl::Convert(desc.blendOp);

    glBlendEquation(func);
    glBlendFunc(src_blend, dest_blend);
}

void OpenGlGraphicsManager::SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) {
    auto draw_pass = reinterpret_cast<const OpenGlDrawPass*>(p_draw_pass);
    DEV_ASSERT(draw_pass);

    glBindFramebuffer(GL_FRAMEBUFFER, draw_pass->handle);

    // @TODO: bind cube map/texture 2d array
    if (!draw_pass->desc.colorAttachments.empty()) {
        const auto resource = draw_pass->desc.colorAttachments[0];
        if (resource->desc.type == AttachmentType::COLOR_CUBE) {
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + p_index,
                                   static_cast<uint32_t>(resource->GetHandle()),
                                   p_mip_level);
        }
    }

    if (const auto depth_attachment = draw_pass->desc.depthAttachment; depth_attachment) {
        if (depth_attachment->desc.type == AttachmentType::SHADOW_CUBE_ARRAY) {
            glFramebufferTextureLayer(GL_FRAMEBUFFER,
                                      GL_DEPTH_ATTACHMENT,
                                      depth_attachment->GetHandle32(),
                                      0,
                                      p_index);
        }
    }

    return;
}

void OpenGlGraphicsManager::UnsetRenderTarget() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGlGraphicsManager::CreateGpuResources() {
    // @TODO: move to renderer
    auto& cache = g_constantCache.cache;
    // @TODO: refactor!
    unsigned int m1 = LoadMTexture(LTC1);
    unsigned int m2 = LoadMTexture(LTC2);
    cache.c_ltc1.handle_gl = MakeTextureResident(m1);
    cache.c_ltc2.handle_gl = MakeTextureResident(m2);

    g_constantCache.update();
}

void OpenGlGraphicsManager::Render() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (m_app->GetSpecification().enableImgui) {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

void OpenGlGraphicsManager::Present() {
    OPTICK_EVENT();

    if (m_app->GetSpecification().enableImgui) {
        GLFWwindow* oldContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(oldContext);
    }

    glfwSwapBuffers(m_window);
}

static void APIENTRY DebugCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei,
                                   const char* message, const void*) {
    switch (id) {
        case 131185:
        case 131204:
            return;
    }

    const char* sourceStr = "GL_DEBUG_SOURCE_OTHER";
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            sourceStr = "GL_DEBUG_SOURCE_API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            sourceStr = "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            sourceStr = "GL_DEBUG_SOURCE_SHADER_COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            sourceStr = "GL_DEBUG_SOURCE_THIRD_PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            sourceStr = "GL_DEBUG_SOURCE_APPLICATION";
            break;
        default:
            break;
    }

    const char* typeStr = "GL_DEBUG_TYPE_OTHER";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            typeStr = "GL_DEBUG_TYPE_ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            typeStr = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            typeStr = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            typeStr = "GL_DEBUG_TYPE_PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            typeStr = "GL_DEBUG_TYPE_PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_MARKER:
            typeStr = "GL_DEBUG_TYPE_MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            typeStr = "GL_DEBUG_TYPE_PUSH_GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            typeStr = "GL_DEBUG_TYPE_POP_GROUP";
            break;
        default:
            break;
    }

    LogLevel level = LOG_LEVEL_NORMAL;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            level = LOG_LEVEL_ERROR;
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            level = LOG_LEVEL_WARN;
            break;
        default:
            break;
    }

    my::LogImpl(level, std::format("[opengl] {}\n\t| id: {} | source: {} | type: {}", message, id, sourceStr, typeStr));
}

}  // namespace my
