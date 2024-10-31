#include "opengl_graphics_manager.h"

#include "assets/image.h"
#include "core/debugger/profiler.h"
#include "core/framework/asset_manager.h"
#include "core/math/geometry.h"
#include "drivers/opengl/opengl_pipeline_state_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"
#include "drivers/opengl/opengl_resources.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "rendering/GpuTexture.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/rendering_dvars.h"
#include "vsinput.glsl.h"

// @TODO: remove
#include "rendering/ltc_matrix.h"
/////

// @TODO: refactor
using namespace my;
using my::rg::RenderGraph;
using my::rg::RenderPass;

/// textures
// @TODO: time to refactor this!!
OldTexture g_albedoVoxel;
OldTexture g_normalVoxel;

// @TODO: refactor
OpenGLMeshBuffers* g_box;
OpenGLMeshBuffers* g_grass;

template<typename T>
static void BufferStorage(GLuint buffer, const std::vector<T>& data) {
    glNamedBufferStorage(buffer, sizeof(T) * data.size(), data.data(), 0);
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

OpenGLGraphicsManager::OpenGLGraphicsManager() : GraphicsManager("OpenGLGraphicsManager", Backend::OPENGL) {
    m_pipelineStateManager = std::make_shared<OpenGLPipelineStateManager>();
}

bool OpenGLGraphicsManager::InitializeImpl() {
    if (gladLoadGL() == 0) {
        LOG_FATAL("[glad] failed to import gl functions");
        return false;
    }

    LOG_VERBOSE("[opengl] renderer: {}", (const char*)glGetString(GL_RENDERER));
    LOG_VERBOSE("[opengl] version: {}", (const char*)glGetString(GL_VERSION));

    if (DVAR_GET_BOOL(r_gpu_validation)) {
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

    SelectRenderGraph();

    ImGui_ImplOpenGL3_Init();
    ImGui_ImplOpenGL3_CreateDeviceObjects();

    CreateGpuResources();

    m_meshes.set_description("GPU-Mesh-Allocator");

    return true;
}

void OpenGLGraphicsManager::Finalize() {
    m_pipelineStateManager->finalize();

    ImGui_ImplOpenGL3_Shutdown();
}

void OpenGLGraphicsManager::SetPipelineStateImpl(PipelineStateName p_name) {
    auto pipeline = reinterpret_cast<OpenGLPipelineState*>(m_pipelineStateManager->find(p_name));

    if (pipeline->rasterizer_desc) {
        const auto cull_mode = pipeline->rasterizer_desc->cull_mode;
        if (cull_mode != m_state_cache.cull_mode) {
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
            m_state_cache.cull_mode = cull_mode;
        }

        const bool front_counter_clockwise = pipeline->rasterizer_desc->front_counter_clockwise;
        if (front_counter_clockwise != m_state_cache.front_counter_clockwise) {
            glFrontFace(front_counter_clockwise ? GL_CCW : GL_CW);
            m_state_cache.front_counter_clockwise = front_counter_clockwise;
        }
    }

    if (pipeline->depth_stencil_desc) {
        {
            const bool enable_depth_test = pipeline->depth_stencil_desc->depth_enabled;
            if (enable_depth_test != m_state_cache.enable_depth_test) {
                if (enable_depth_test) {
                    glEnable(GL_DEPTH_TEST);
                } else {
                    glDisable(GL_DEPTH_TEST);
                }
                m_state_cache.enable_depth_test = enable_depth_test;
            }

            if (enable_depth_test) {
                const auto func = pipeline->depth_stencil_desc->depth_func;
                if (func != m_state_cache.depth_func) {
                    glDepthFunc(gl::convert(func));
                    m_state_cache.depth_func = func;
                }
            }
        }
        {
            const bool enable_stencil_test = pipeline->depth_stencil_desc->stencil_enabled;
            if (enable_stencil_test != m_state_cache.enable_stencil_test) {
                if (enable_stencil_test) {
                    glEnable(GL_STENCIL_TEST);
                } else {
                    glDisable(GL_STENCIL_TEST);
                }
                m_state_cache.enable_stencil_test = enable_stencil_test;
            }

            if (enable_stencil_test) {
                switch (pipeline->depth_stencil_desc->op) {
                    case DepthStencilOpDesc::ALWAYS:
                        glStencilFunc(GL_ALWAYS, 0, 0xFF);
                        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                        m_state_cache.stencil_func = GL_ALWAYS;
                        break;
                    case DepthStencilOpDesc::Z_PASS:
                        glStencilFunc(GL_ALWAYS, 0, 0xFF);
                        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                        m_state_cache.stencil_func = GL_ALWAYS;
                        break;
                    case DepthStencilOpDesc::EQUAL:
                        glStencilFunc(GL_EQUAL, 0, 0xFF);
                        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                        m_state_cache.stencil_func = GL_EQUAL;
                        break;
                    default:
                        CRASH_NOW();
                        break;
                }
            }
        }
    }

    glUseProgram(pipeline->program_id);
}

void OpenGLGraphicsManager::Clear(const DrawPass* p_draw_pass, uint32_t p_flags, float* p_clear_color, int p_index) {
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

void OpenGLGraphicsManager::SetViewport(const Viewport& p_viewport) {
    if (p_viewport.topLeftY) {
        LOG_FATAL("TODO: adjust to bottom left y");
    }

    glViewport(p_viewport.topLeftX,
               p_viewport.topLeftY,
               p_viewport.width,
               p_viewport.height);
}

const MeshBuffers* OpenGLGraphicsManager::CreateMesh(const MeshComponent& p_mesh) {
    RID rid = m_meshes.make_rid();
    OpenGLMeshBuffers* mesh_buffers = m_meshes.get_or_null(rid);
    p_mesh.gpu_resource = mesh_buffers;

    auto createMesh_data = [](const MeshComponent& p_mesh, OpenGLMeshBuffers& p_out_mesh) {
        const bool has_normals = !p_mesh.normals.empty();
        const bool has_uvs = !p_mesh.texcoords_0.empty();
        const bool has_tangents = !p_mesh.tangents.empty();
        const bool has_joints = !p_mesh.joints_0.empty();
        const bool has_weights = !p_mesh.weights_0.empty();

        int vbo_count = 1 + has_normals + has_uvs + has_tangents + has_joints + has_weights;
        DEV_ASSERT(vbo_count <= array_length(p_out_mesh.vbos));

        glGenVertexArrays(1, &p_out_mesh.vao);

        // @TODO: fix this hack
        glGenBuffers(1, &p_out_mesh.ebo);
        glGenBuffers(6, p_out_mesh.vbos);

        int slot = -1;
        glBindVertexArray(p_out_mesh.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_out_mesh.ebo);

        slot = get_position_slot();
        // @TODO: refactor these
        BindToSlot(p_out_mesh.vbos[slot], slot, 3);
        BufferStorage(p_out_mesh.vbos[slot], p_mesh.positions);

        if (has_normals) {
            slot = get_normal_slot();
            BindToSlot(p_out_mesh.vbos[slot], slot, 3);
            BufferStorage(p_out_mesh.vbos[slot], p_mesh.normals);
        }
        if (has_uvs) {
            slot = get_uv_slot();
            BindToSlot(p_out_mesh.vbos[slot], slot, 2);
            BufferStorage(p_out_mesh.vbos[slot], p_mesh.texcoords_0);
        }
        if (has_tangents) {
            slot = get_tangent_slot();
            BindToSlot(p_out_mesh.vbos[slot], slot, 3);
            BufferStorage(p_out_mesh.vbos[slot], p_mesh.tangents);
        }
        if (has_joints) {
            slot = get_bone_id_slot();
            BindToSlot(p_out_mesh.vbos[slot], slot, 4);
            BufferStorage(p_out_mesh.vbos[slot], p_mesh.joints_0);
            DEV_ASSERT(!p_mesh.weights_0.empty());
            slot = get_bone_weight_slot();
            BindToSlot(p_out_mesh.vbos[slot], slot, 4);
            BufferStorage(p_out_mesh.vbos[slot], p_mesh.weights_0);
        }

        BufferStorage(p_out_mesh.ebo, p_mesh.indices);
        p_out_mesh.indexCount = static_cast<uint32_t>(p_mesh.indices.size());

        glBindVertexArray(0);
    };

    createMesh_data(p_mesh, *mesh_buffers);
    return mesh_buffers;
}

void OpenGLGraphicsManager::SetMesh(const MeshBuffers* p_mesh) {
    auto mesh = reinterpret_cast<const OpenGLMeshBuffers*>(p_mesh);
    glBindVertexArray(mesh->vao);
}

void OpenGLGraphicsManager::DrawElements(uint32_t p_count, uint32_t p_offset) {
    glDrawElements(GL_TRIANGLES, p_count, GL_UNSIGNED_INT, (void*)(p_offset * sizeof(uint32_t)));
}

void OpenGLGraphicsManager::DrawElementsInstanced(uint32_t p_instance_count, uint32_t p_count, uint32_t p_offset) {
    glDrawElementsInstanced(GL_TRIANGLES, p_count, GL_UNSIGNED_INT, (void*)(p_offset * sizeof(uint32_t)), p_instance_count);
}

void OpenGLGraphicsManager::Dispatch(uint32_t p_num_groups_x, uint32_t p_num_groups_y, uint32_t p_num_groups_z) {
    glDispatchCompute(p_num_groups_x, p_num_groups_y, p_num_groups_z);
    // @TODO: this probably shouldn't be here
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void OpenGLGraphicsManager::SetUnorderedAccessView(uint32_t p_slot, GpuTexture* p_texture) {
    GLuint handle = p_texture ? p_texture->GetHandle32() : 0;

    glBindImageTexture(p_slot, handle, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);
}

std::shared_ptr<GpuBuffer> OpenGLGraphicsManager::CreateBuffer(const GpuBufferDesc& p_desc) {
    unused(p_desc);
    return nullptr;
}

std::shared_ptr<ConstantBufferBase> OpenGLGraphicsManager::CreateConstantBuffer(int p_slot, size_t p_capacity) {
    auto buffer = std::make_shared<OpenGLUniformBuffer>(p_slot, p_capacity);
    GLuint handle = 0;
    glGenBuffers(1, &handle);
    glBindBuffer(GL_UNIFORM_BUFFER, handle);
    glBufferData(GL_UNIFORM_BUFFER, p_capacity, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, p_slot, handle);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    buffer->handle = handle;
    return buffer;
}

void OpenGLGraphicsManager::UpdateConstantBuffer(const ConstantBufferBase* p_buffer, const void* p_data, size_t p_size) {
    // ERR_FAIL_INDEX(p_size, p_buffer->get_capacity());
    auto buffer = reinterpret_cast<const OpenGLUniformBuffer*>(p_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer->handle);
    glBufferData(GL_UNIFORM_BUFFER, p_size, p_data, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void OpenGLGraphicsManager::BindConstantBufferRange(const ConstantBufferBase* p_buffer, uint32_t p_size, uint32_t p_offset) {
    ERR_FAIL_INDEX(p_offset + p_offset, p_buffer->get_capacity() + 1);
    auto buffer = reinterpret_cast<const OpenGLUniformBuffer*>(p_buffer);
    glBindBufferRange(GL_UNIFORM_BUFFER, p_buffer->get_slot(), buffer->handle, p_offset, p_size);
}

void OpenGLGraphicsManager::BindTexture(Dimension p_dimension, uint64_t p_handle, int p_slot) {
    if (p_handle == 0) {
        return;
    }

    const GLuint texture_type = gl::convert_dimension(p_dimension);
    glActiveTexture(GL_TEXTURE0 + p_slot);
    glBindTexture(texture_type, static_cast<GLuint>(p_handle));
}

void OpenGLGraphicsManager::UnbindTexture(Dimension p_dimension, int p_slot) {
    const GLuint texture_type = gl::convert_dimension(p_dimension);

    glActiveTexture(GL_TEXTURE0 + p_slot);
    glBindTexture(texture_type, 0);
}

std::shared_ptr<GpuTexture> OpenGLGraphicsManager::CreateTexture(const GpuTextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
    GLuint texture_id = 0;
    glGenTextures(1, &texture_id);

    GLenum texture_type = gl::convert_dimension(p_texture_desc.dimension);
    GLenum internal_format = gl::convert_internal_format(p_texture_desc.format);
    GLenum format = gl::convert_format(p_texture_desc.format);
    GLenum data_type = gl::convert_data_type(p_texture_desc.format);

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
                         p_texture_desc.initial_data);
        } break;
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
                             p_texture_desc.initial_data);
            }
        } break;
        default:
            CRASH_NOW();
            break;
    }

    gl::set_sampler(texture_type, p_sampler_desc);
    if (p_texture_desc.misc_flags & RESOURCE_MISC_GENERATE_MIPS) {
        glGenerateMipmap(texture_type);
    }

    glBindTexture(texture_type, 0);

    GLuint64 resident_id = glGetTextureHandleARB(texture_id);
    glMakeTextureHandleResidentARB(resident_id);

    auto texture = std::make_shared<OpenGLTexture>(p_texture_desc);
    texture->handle = texture_id;
    texture->resident_handle = resident_id;
    return texture;
}

std::shared_ptr<DrawPass> OpenGLGraphicsManager::CreateDrawPass(const DrawPassDesc& p_desc) {
    auto draw_pass = std::make_shared<OpenGLSubpass>();
    draw_pass->exec_func = p_desc.exec_func;
    draw_pass->color_attachments = p_desc.color_attachments;
    draw_pass->depth_attachment = p_desc.depth_attachment;
    GLuint fbo_handle = 0;

    const int num_depth_attachment = p_desc.depth_attachment != nullptr;
    const int num_color_attachment = (int)p_desc.color_attachments.size();
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

            const auto& color_attachment = p_desc.color_attachments[idx];
            uint32_t texture_handle = static_cast<uint32_t>(color_attachment->texture->GetHandle());
            switch (color_attachment->desc.type) {
                case AttachmentType::COLOR_2D: {
                    glFramebufferTexture2D(GL_FRAMEBUFFER,  // target
                                           attachment,      // attachment
                                           GL_TEXTURE_2D,   // texture target
                                           texture_handle,  // texture
                                           0                // level
                    );
                } break;
                case AttachmentType::COLOR_CUBE_MAP: {
                } break;
                default:
                    CRASH_NOW();
                    break;
            }
        }

        glDrawBuffers(num_color_attachment, attachments.data());
    }

    if (auto depth_attachment = p_desc.depth_attachment; depth_attachment) {
        uint32_t texture_handle = static_cast<uint32_t>(depth_attachment->texture->GetHandle());
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
            case AttachmentType::SHADOW_CUBE_MAP: {
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

void OpenGLGraphicsManager::SetStencilRef(uint32_t p_ref) {
    glStencilFunc(m_state_cache.stencil_func, p_ref, 0xFF);
}

void OpenGLGraphicsManager::SetRenderTarget(const DrawPass* p_draw_pass, int p_index, int p_mip_level) {
    auto draw_pass = reinterpret_cast<const OpenGLSubpass*>(p_draw_pass);
    if (!draw_pass || draw_pass->handle == 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, draw_pass->handle);

    // @TODO: bind cube map/texture 2d array
    if (!draw_pass->color_attachments.empty()) {
        const auto resource = draw_pass->color_attachments[0];
        if (resource->desc.type == AttachmentType::COLOR_CUBE_MAP) {
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + p_index,
                                   static_cast<uint32_t>(resource->texture->GetHandle()),
                                   p_mip_level);
        }
    }

    if (const auto depth_attachment = draw_pass->depth_attachment; depth_attachment) {
        if (depth_attachment->desc.type == AttachmentType::SHADOW_CUBE_MAP) {
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + p_index,
                                   static_cast<uint32_t>(depth_attachment->texture->GetHandle()),
                                   p_mip_level);
        }
    }

    return;
}

void OpenGLGraphicsManager::UnsetRenderTarget() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// @TODO: refactor this, instead off iterate through all the meshes, find a better way
void OpenGLGraphicsManager::OnSceneChange(const Scene& p_scene) {
    for (auto [entity, mesh] : p_scene.m_MeshComponents) {
        if (mesh.gpu_resource != nullptr) {
            continue;
        }

        CreateMesh(mesh);
    }

    g_constantCache.update();
}

void OpenGLGraphicsManager::CreateGpuResources() {
    // @TODO: appropriate sampler
    auto grass_image = AssetManager::GetSingleton().loadImageSync(FilePath{ "@res://images/grass.png" })->Get();

    // @TODO: move to renderer
    g_grass = (OpenGLMeshBuffers*)CreateMesh(MakeGrassBillboard());
    g_box = (OpenGLMeshBuffers*)CreateMesh(MakeBoxMesh());

    const int voxelSize = DVAR_GET_INT(r_voxel_size);

    /// create voxel image
    {
        Texture3DCreateInfo info;
        info.wrapS = info.wrapT = info.wrapR = GL_CLAMP_TO_BORDER;
        info.size = voxelSize;
        info.minFilter = GL_LINEAR_MIPMAP_LINEAR;
        info.magFilter = GL_NEAREST;
        info.mipLevel = math::LogTwo(voxelSize);
        info.format = GL_RGBA16F;

        g_albedoVoxel.create3DEmpty(info);
        g_normalVoxel.create3DEmpty(info);
    }

    auto& cache = g_constantCache.cache;

    // @TODO: delete!
    unsigned int m1 = LoadMTexture(LTC1);
    unsigned int m2 = LoadMTexture(LTC2);
    cache.u_ltc_1 = MakeTextureResident(m1);
    cache.u_ltc_2 = MakeTextureResident(m2);

    cache.c_voxel_map = MakeTextureResident(g_albedoVoxel.GetHandle());
    cache.c_voxel_normal_map = MakeTextureResident(g_normalVoxel.GetHandle());

    cache.u_grass_base_color = grass_image->gpu_texture->GetResidentHandle();

    // @TODO: refactor
    auto make_resident = [&](RenderTargetResourceName p_name, uint64_t& p_out_id) {
        std::shared_ptr<RenderTarget> resource = FindRenderTarget(p_name);
        if (resource) {
            p_out_id = resource->texture->GetResidentHandle();
        } else {
            p_out_id = 0;
        }
    };

    make_resident(RESOURCE_TONE, cache.c_tone_image);
    make_resident(RESOURCE_GBUFFER_DEPTH, cache.u_gbuffer_depth_map);
    make_resident(RESOURCE_ENV_SKYBOX_CUBE_MAP, cache.c_env_map);
    make_resident(RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP, cache.c_diffuse_irradiance_map);
    make_resident(RESOURCE_ENV_PREFILTER_CUBE_MAP, cache.c_prefiltered_map);
    make_resident(RESOURCE_BRDF, cache.c_brdf_map);
    make_resident(RESOURCE_BLOOM_0, cache.u_final_bloom);

    g_constantCache.update();
}

void OpenGLGraphicsManager::Render() {
    OPTICK_EVENT();

    m_renderGraph.execute();

    // @TODO: move it somewhere else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    {
        OPTICK_EVENT("ImGui_ImplOpenGL3_RenderDrawData");
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
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
