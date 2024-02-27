#include "opengl_graphics_manager.h"

// @TODO: remove
#include <random>
/////

#include "assets/image.h"
#include "core/base/rid_owner.h"
#include "core/debugger/profiler.h"
#include "core/framework/asset_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/geometry.h"
#include "drivers/opengl/opengl_pipeline_state_manager.h"
#include "drivers/opengl/opengl_subpass.h"
#include "drivers/opengl/opengl_texture.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "rendering/gl_utils.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/render_graphs.h"
#include "rendering/renderer.h"
#include "rendering/rendering_dvars.h"
#include "vsinput.glsl.h"

using my::rg::RenderGraph;
using my::rg::RenderPass;

/// textures
// @TODO: time to refactor this!!
GpuTexture g_albedoVoxel;
GpuTexture g_normalVoxel;

// @TODO: refactor
MeshData g_box;
MeshData g_skybox;
MeshData g_billboard;

// @TODO: fix this
my::RIDAllocator<MeshData> g_meshes;

static GLuint g_noiseTexture;

using namespace my;

// @TODO: refactor
template<typename T>
static void buffer_storage(GLuint buffer, const std::vector<T>& data) {
    glNamedBufferStorage(buffer, sizeof(T) * data.size(), data.data(), 0);
}

static inline void bind_to_slot(GLuint buffer, int slot, int size) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(slot, size, GL_FLOAT, GL_FALSE, size * sizeof(float), 0);
    glEnableVertexAttribArray(slot);
}

namespace my {

static void APIENTRY gl_debug_callback(GLenum, GLenum, unsigned int, GLenum, GLsizei, const char*, const void*);

bool OpenGLGraphicsManager::initialize() {
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
            glDebugMessageCallback(gl_debug_callback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            LOG_VERBOSE("[opengl] debug callback enabled");
        }
    }

    select_render_graph();

    ImGui_ImplOpenGL3_Init();
    ImGui_ImplOpenGL3_CreateDeviceObjects();

    createGpuResources();

    g_meshes.set_description("GPU-Mesh-Allocator");

    m_render_data = std::make_shared<RenderData>();

    m_pipeline_state_manager = std::make_shared<OpenGLPipelineStateManager>();
    return m_pipeline_state_manager->initialize();
}

void OpenGLGraphicsManager::finalize() {
    m_pipeline_state_manager->finalize();

    destroyGpuResources();

    ImGui_ImplOpenGL3_Shutdown();
}

void OpenGLGraphicsManager::set_pipeline_state_impl(PipelineStateName p_name) {
    auto pipeline = reinterpret_cast<OpenGLPipelineState*>(m_pipeline_state_manager->find(p_name));
    glUseProgram(pipeline->program_id);
}

static void create_mesh_data(const MeshComponent& mesh, MeshData& out_mesh) {
    const bool has_normals = !mesh.normals.empty();
    const bool has_uvs = !mesh.texcoords_0.empty();
    const bool has_tangents = !mesh.tangents.empty();
    const bool has_joints = !mesh.joints_0.empty();
    const bool has_weights = !mesh.weights_0.empty();

    int vbo_count = 1 + has_normals + has_uvs + has_tangents + has_joints + has_weights;
    DEV_ASSERT(vbo_count <= array_length(out_mesh.vbos));

    glGenVertexArrays(1, &out_mesh.vao);

    // @TODO: fix this hack
    glGenBuffers(1, &out_mesh.ebo);
    glGenBuffers(6, out_mesh.vbos);

    int slot = -1;
    glBindVertexArray(out_mesh.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out_mesh.ebo);

    slot = get_position_slot();
    // @TODO: refactor these
    bind_to_slot(out_mesh.vbos[slot], slot, 3);
    buffer_storage(out_mesh.vbos[slot], mesh.positions);

    if (has_normals) {
        slot = get_normal_slot();
        bind_to_slot(out_mesh.vbos[slot], slot, 3);
        buffer_storage(out_mesh.vbos[slot], mesh.normals);
    }
    if (has_uvs) {
        slot = get_uv_slot();
        bind_to_slot(out_mesh.vbos[slot], slot, 2);
        buffer_storage(out_mesh.vbos[slot], mesh.texcoords_0);
    }
    if (has_tangents) {
        slot = get_tangent_slot();
        bind_to_slot(out_mesh.vbos[slot], slot, 3);
        buffer_storage(out_mesh.vbos[slot], mesh.tangents);
    }
    if (has_joints) {
        slot = get_bone_id_slot();
        bind_to_slot(out_mesh.vbos[slot], slot, 4);
        buffer_storage(out_mesh.vbos[slot], mesh.joints_0);
        DEV_ASSERT(!mesh.weights_0.empty());
        slot = get_bone_weight_slot();
        bind_to_slot(out_mesh.vbos[slot], slot, 4);
        buffer_storage(out_mesh.vbos[slot], mesh.weights_0);
    }

    buffer_storage(out_mesh.ebo, mesh.indices);
    out_mesh.count = static_cast<uint32_t>(mesh.indices.size());

    glBindVertexArray(0);
}

std::shared_ptr<Texture> OpenGLGraphicsManager::create_texture(const TextureDesc& p_texture_desc, const SamplerDesc& p_sampler_desc) {
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

void OpenGLGraphicsManager::on_scene_change(const Scene& p_scene) {
    for (size_t idx = 0; idx < p_scene.get_count<MeshComponent>(); ++idx) {
        const MeshComponent& mesh = p_scene.get_component_array<MeshComponent>()[idx];
        if (mesh.gpu_resource.is_valid()) {
            ecs::Entity entity = p_scene.get_entity<MeshComponent>(idx);
            const NameComponent& name = *p_scene.get_component<NameComponent>(entity);
            LOG_WARN("[begin_scene] mesh '{}' (idx: {}) already has gpu resource", name.get_name(), idx);
            continue;
        }
        RID rid = g_meshes.make_rid();
        mesh.gpu_resource = rid;
        create_mesh_data(mesh, *g_meshes.get_or_null(rid));
    }

    g_constantCache.Update();
}

// @TODO: refactor
static void create_ssao_resource() {
    // @TODO: save
    // generate sample kernel
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);  // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec4> ssaoKernel;
    const int kernelSize = DVAR_GET_INT(r_ssaoKernelSize);
    for (int i = 0; i < kernelSize; ++i) {
        // [-1, 1], [-1, 1], [0, 1]
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / kernelSize;

        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.emplace_back(vec4(sample, 0.0f));
    }

    memset(&g_constantCache.cache.c_ssao_kernels, 0, sizeof(g_constantCache.cache.c_ssao_kernels));
    memcpy(&g_constantCache.cache.c_ssao_kernels, ssaoKernel.data(), sizeof(ssaoKernel.front()) * ssaoKernel.size());

    // generate noise texture
    const int noiseSize = DVAR_GET_INT(r_ssaoNoiseSize);

    std::vector<glm::vec3> ssaoNoise;
    for (int i = 0; i < noiseSize * noiseSize; ++i) {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
        noise = glm::normalize(noise);
        ssaoNoise.emplace_back(noise);
    }
    unsigned int noiseTexture;
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, noiseSize, noiseSize, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    g_constantCache.cache.c_kernel_noise_map = ::gl::MakeTextureResident(noiseTexture);
    g_noiseTexture = noiseTexture;
}

void OpenGLGraphicsManager::createGpuResources() {
    create_ssao_resource();

    R_Alloc_Cbuffers();

    // create a dummy box data
    create_mesh_data(make_plane_mesh(0.3f), g_billboard);
    create_mesh_data(make_box_mesh(), g_box);
    create_mesh_data(make_sky_box_mesh(), g_skybox);

    const int voxelSize = DVAR_GET_INT(r_voxel_size);

    /// create voxel image
    {
        Texture3DCreateInfo info;
        info.wrapS = info.wrapT = info.wrapR = GL_CLAMP_TO_BORDER;
        info.size = voxelSize;
        info.minFilter = GL_LINEAR_MIPMAP_LINEAR;
        info.magFilter = GL_NEAREST;
        info.mipLevel = math::log_two(voxelSize);
        info.format = GL_RGBA16F;

        g_albedoVoxel.create3DEmpty(info);
        g_normalVoxel.create3DEmpty(info);
    }

    // create box quad
    R_CreateQuad();

    auto& cache = g_constantCache.cache;
    cache.c_voxel_map = ::gl::MakeTextureResident(g_albedoVoxel.GetHandle());
    cache.c_voxel_normal_map = ::gl::MakeTextureResident(g_normalVoxel.GetHandle());

    // @TODO: refactor
    auto make_resident = [&](const std::string& name, uint64_t& id) {
        std::shared_ptr<RenderTarget> resource = find_resource(name);
        if (resource) {
            id = resource->get_resident_handle();
        } else {
            id = 0;
        }
    };

    make_resident(RT_RES_SHADOW_MAP, cache.c_shadow_map);
    make_resident(RT_RES_SSAO, cache.c_ssao_map);
    make_resident(RT_RES_FXAA, cache.c_fxaa_image);
    make_resident(RT_RES_GBUFFER_POSITION, cache.c_gbuffer_position_metallic_map);
    make_resident(RT_RES_GBUFFER_NORMAL, cache.c_gbuffer_normal_roughness_map);
    make_resident(RT_RES_GBUFFER_BASE_COLOR, cache.c_gbuffer_albedo_map);
    make_resident(RT_RES_GBUFFER_DEPTH, cache.c_gbuffer_depth_map);
    make_resident(RT_RES_LIGHTING, cache.c_fxaa_input_image);
    make_resident(RT_ENV_SKYBOX_CUBE_MAP, cache.c_env_map);
    make_resident(RT_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP, cache.c_diffuse_irradiance_map);
    make_resident(RT_ENV_PREFILTER_CUBE_MAP, cache.c_prefiltered_map);
    make_resident(RT_BRDF, cache.c_brdf_map);

    g_constantCache.Update();
}

uint64_t OpenGLGraphicsManager::get_final_image() const {
    switch (m_method) {
        case RenderGraph::VXGI:
            return find_resource(RT_RES_FINAL)->get_handle();
        case RenderGraph::BASE_COLOR:
            return find_resource(RT_RES_BASE_COLOR)->get_handle();
        case RenderGraph::DUMMY:
            return find_resource(RT_RES_FINAL)->get_handle();
        default:
            CRASH_NOW();
            return 0;
    }
}

std::shared_ptr<RenderTarget> OpenGLGraphicsManager::create_resource(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler) {
    DEV_ASSERT(m_resource_lookup.find(p_desc.name) == m_resource_lookup.end());
    std::shared_ptr<RenderTarget> resource = std::make_shared<RenderTarget>(p_desc);

    TextureDesc texture_desc{};
    switch (p_desc.type) {
        case RT_COLOR_ATTACHMENT_2D:
        case RT_DEPTH_ATTACHMENT_2D:
        case RT_SHADOW_2D:
            texture_desc.dimension = Dimension::TEXTURE_2D;
            break;
        case RT_SHADOW_CUBE_MAP:
        case RT_COLOR_ATTACHMENT_CUBE_MAP:
            texture_desc.dimension = Dimension::TEXTURE_CUBE;
            break;
        default:
            break;
    }
    texture_desc.format = p_desc.format;
    texture_desc.width = p_desc.width;
    texture_desc.height = p_desc.height;
    texture_desc.initial_data = nullptr;
    texture_desc.misc_flags = 0;
    texture_desc.bind_flags = 0;
    texture_desc.mip_levels = 1;
    texture_desc.array_size = 1;
    if (p_desc.gen_mipmap) {
        texture_desc.misc_flags |= RESOURCE_MISC_GENERATE_MIPS;
        texture_desc.bind_flags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    }

    resource->m_texture = create_texture(texture_desc, p_sampler);

    m_resource_lookup[resource->m_desc.name] = resource;
    return resource;
}

std::shared_ptr<Subpass> OpenGLGraphicsManager::create_subpass(const SubpassDesc& p_desc) {
    auto subpass = std::make_shared<OpenGLSubpass>();
    subpass->func = p_desc.func;
    subpass->color_attachments = p_desc.color_attachments;
    subpass->depth_attachment = p_desc.depth_attachment;
    GLuint fbo_handle = 0;

    const int num_depth_attachment = p_desc.depth_attachment != nullptr;
    const int num_color_attachment = (int)p_desc.color_attachments.size();
    if (!num_depth_attachment && !num_color_attachment) {
        return subpass;
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
            switch (color_attachment->get_desc().type) {
                case RT_COLOR_ATTACHMENT_2D: {
                    glFramebufferTexture2D(
                        GL_FRAMEBUFFER,                  // target
                        attachment,                      // attachment
                        GL_TEXTURE_2D,                   // texture target
                        color_attachment->get_handle(),  // texture
                        0                                // level
                    );
                } break;
                case RT_COLOR_ATTACHMENT_CUBE_MAP: {
                } break;
                default:
                    CRASH_NOW();
                    break;
            }
        }

        glDrawBuffers(num_color_attachment, attachments.data());
    }

    // @TODO: move it to bind and unbind
    if (auto depth_attachment = p_desc.depth_attachment; depth_attachment) {
        switch (depth_attachment->get_desc().type) {
            case RT_SHADOW_2D:
            case RT_DEPTH_ATTACHMENT_2D: {
                glFramebufferTexture2D(GL_FRAMEBUFFER,                  // target
                                       GL_DEPTH_ATTACHMENT,             // attachment
                                       GL_TEXTURE_2D,                   // texture target
                                       depth_attachment->get_handle(),  // texture
                                       0                                // level
                );
            } break;
            case RT_SHADOW_CUBE_MAP: {
                glFramebufferTexture(GL_FRAMEBUFFER,
                                     GL_DEPTH_ATTACHMENT,
                                     depth_attachment->get_handle(),
                                     0);
            } break;
            default:
                CRASH_NOW();
                break;
        }
    }

    DEV_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    subpass->m_handle = fbo_handle;
    return subpass;
}

void OpenGLGraphicsManager::fill_material_constant_buffer(const MaterialComponent* material, MaterialConstantBuffer& cb) {
    cb.c_albedo_color = material->base_color;
    cb.c_metallic = material->metallic;
    cb.c_roughness = material->roughness;

    auto set_texture = [&](int idx, sampler2D& out_handle) {
        if (!material->textures[idx].enabled) {
            return false;
        }

        ImageHandle* handle = material->textures[idx].image;
        if (!handle) {
            return false;
        }

        Image* image = handle->get();
        if (!image) {
            return false;
        }

        const OpenGLTexture* texture = reinterpret_cast<OpenGLTexture*>(image->gpu_texture.get());
        if (!texture) {
            return false;
        }

        out_handle = texture->resident_handle;
        return true;
    };

    cb.c_has_albedo_map = set_texture(MaterialComponent::TEXTURE_BASE, cb.c_albedo_map);
    cb.c_has_normal_map = set_texture(MaterialComponent::TEXTURE_NORMAL, cb.c_normal_map);
    cb.c_has_pbr_map = set_texture(MaterialComponent::TEXTURE_METALLIC_ROUGHNESS, cb.c_pbr_map);
}

void OpenGLGraphicsManager::render() {
    OPTICK_EVENT();

    // @TODO: move outside
    Scene& scene = SceneManager::singleton().get_scene();
    renderer::fill_constant_buffers(scene);

    m_render_data->update(&scene);

    g_perFrameCache.Update();
    m_render_graph.execute();

    // @TODO: move it somewhere else
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    {
        OPTICK_EVENT("ImGui_ImplOpenGL3_RenderDrawData");
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

void OpenGLGraphicsManager::destroyGpuResources() {
    R_Destroy_Cbuffers();

    glDeleteTextures(1, &g_noiseTexture);
}

static void APIENTRY gl_debug_callback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei,
                                       const char* message, const void*) {
    switch (id) {
        case 131185:
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

    my::log_impl(level, std::format("[opengl] {}\n\t| id: {} | source: {} | type: {}", message, id, sourceStr, typeStr));
}

}  // namespace my
