#include "graphics_manager.h"

#include "core/debugger/profiler.h"
#include "core/math/frustum.h"
#include "core/math/matrix_transform.h"
#include "drivers/d3d11/d3d11_graphics_manager.h"
#include "drivers/empty/empty_graphics_manager.h"
#include "drivers/opengl/opengl_graphics_manager.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"

// @TODO: refactor
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace my {

template<typename T>
static auto CreateUniform(GraphicsManager& p_graphics_manager, uint32_t p_max_count) {
    static_assert(sizeof(T) % 256 == 0);
    return p_graphics_manager.CreateConstantBuffer(T::GetUniformBufferSlot(), sizeof(T) * p_max_count);
}

ConstantBuffer<PerFrameConstantBuffer> g_per_frame_cache;
ConstantBuffer<PerSceneConstantBuffer> g_constantCache;
ConstantBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;
ConstantBuffer<PointShadowConstantBuffer> g_point_shadow_cache;
ConstantBuffer<EnvConstantBuffer> g_env_cache;
ConstantBuffer<ParticleConstantBuffer> g_particleCache;

template<typename T>
static void create_uniform_buffer(ConstantBuffer<T>& p_buffer) {
    constexpr int slot = T::GetUniformBufferSlot();
    p_buffer.buffer = GraphicsManager::GetSingleton().CreateConstantBuffer(slot, sizeof(T));
}

bool GraphicsManager::Initialize() {
    if (!InitializeImpl()) {
        return false;
    }

    // @TODO: refactor
    m_context.batch_uniform = ::my::CreateUniform<PerBatchConstantBuffer>(*this, 4096 * 16);
    m_context.pass_uniform = ::my::CreateUniform<PerPassConstantBuffer>(*this, 32);
    m_context.material_uniform = ::my::CreateUniform<MaterialConstantBuffer>(*this, 2048 * 16);
    m_context.bone_uniform = ::my::CreateUniform<BoneConstantBuffer>(*this, 16);

    create_uniform_buffer<PerFrameConstantBuffer>(g_per_frame_cache);
    create_uniform_buffer<PerSceneConstantBuffer>(g_constantCache);
    create_uniform_buffer<DebugDrawConstantBuffer>(g_debug_draw_cache);
    create_uniform_buffer<PointShadowConstantBuffer>(g_point_shadow_cache);
    create_uniform_buffer<EnvConstantBuffer>(g_env_cache);
    create_uniform_buffer<ParticleConstantBuffer>(g_particleCache);

    DEV_ASSERT(m_pipelineStateManager);

    if (!m_pipelineStateManager->Initialize()) {
        return false;
    }

    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = FindRenderTarget(p_name);
        if (!resource) {
            return;
        }

        BindTexture(p_dimension, resource->texture->GetHandle(), p_slot);
    };

    // bind common textures
    bind_slot(RESOURCE_HIGHLIGHT_SELECT, u_selection_highlight_slot);
    bind_slot(RESOURCE_GBUFFER_BASE_COLOR, u_gbuffer_base_color_map_slot);
    bind_slot(RESOURCE_GBUFFER_POSITION, u_gbuffer_position_map_slot);
    bind_slot(RESOURCE_GBUFFER_NORMAL, u_gbuffer_normal_map_slot);
    bind_slot(RESOURCE_GBUFFER_MATERIAL, u_gbuffer_material_map_slot);

    return true;
}

void GraphicsManager::EventReceived(std::shared_ptr<Event> event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(event.get()); e) {
        OnSceneChange(*e->GetScene());
    }
    if (ResizeEvent* e = dynamic_cast<ResizeEvent*>(event.get()); e) {
        OnWindowResize(e->GetWidth(), e->GetHeight());
    }
}

std::shared_ptr<GraphicsManager> GraphicsManager::Create() {
    const std::string& backend = DVAR_GET_STRING(r_backend);

    if (backend == "opengl") {
        return std::make_shared<OpenGLGraphicsManager>();
    } else if (backend == "d3d11") {
        return std::make_shared<D3d11GraphicsManager>();
    }
    return std::make_shared<EmptyGraphicsManager>(Backend::EMPTY);
}

void GraphicsManager::SetPipelineState(PipelineStateName p_name) {
    if (m_lastPipelineName != p_name) {
        SetPipelineStateImpl(p_name);
        m_lastPipelineName = p_name;
    }
}

void GraphicsManager::RequestTexture(ImageHandle* p_handle, OnTextureLoadFunc p_func) {
    m_loadedImages.push(ImageTask{ p_handle, p_func });
}

void GraphicsManager::Update(Scene& p_scene) {
    OPTICK_EVENT();

    Cleanup();

    UpdateConstants(p_scene);
    UpdateParticles(p_scene);
    UpdateLights(p_scene);
    UpdateVoxelPass(p_scene);
    UpdateMainPass(p_scene);

    // update uniform
    UpdateConstantBuffer(m_context.batch_uniform.get(), m_context.batch_cache.buffer);
    UpdateConstantBuffer(m_context.pass_uniform.get(), m_context.pass_cache);
    UpdateConstantBuffer(m_context.material_uniform.get(), m_context.material_cache.buffer);
    UpdateConstantBuffer(m_context.bone_uniform.get(), m_context.bone_cache.buffer);

    g_per_frame_cache.update();
    // update uniform

    // @TODO: make it a function
    auto loaded_images = m_loadedImages.pop_all();
    while (!loaded_images.empty()) {
        auto task = loaded_images.front();
        loaded_images.pop();
        DEV_ASSERT(task.handle->state == ASSET_STATE_READY);
        Image* image = task.handle->Get();
        DEV_ASSERT(image);

        GpuTextureDesc texture_desc{};
        SamplerDesc sampler_desc{};
        renderer::fill_texture_and_sampler_desc(image, texture_desc, sampler_desc);

        image->gpu_texture = CreateTexture(texture_desc, sampler_desc);
        if (task.func) {
            task.func(task.handle->Get());
        }
    }

    Render();
}

void GraphicsManager::SelectRenderGraph() {
    std::string method(DVAR_GET_STRING(r_render_graph));
    if (method == "vxgi") {
        m_method = RenderGraph::VXGI;
    } else {
        m_method = RenderGraph::DEFAULT;
    }

    // force to default
    if (m_backend == Backend::D3D11) {
        m_method = RenderGraph::DEFAULT;
    }

    switch (m_method) {
        case RenderGraph::VXGI:
            createRenderGraphVxgi(m_renderGraph);
            break;
        default:
            createRenderGraphDefault(m_renderGraph);
            break;
    }
}

std::shared_ptr<RenderTarget> GraphicsManager::CreateRenderTarget(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler) {
    DEV_ASSERT(m_resourceLookup.find(p_desc.name) == m_resourceLookup.end());
    std::shared_ptr<RenderTarget> resource = std::make_shared<RenderTarget>(p_desc);

    // @TODO: this part need rework
    GpuTextureDesc texture_desc{};
    texture_desc.array_size = 1;

    switch (p_desc.type) {
        case AttachmentType::COLOR_2D:
        case AttachmentType::DEPTH_2D:
        case AttachmentType::DEPTH_STENCIL_2D:
        case AttachmentType::SHADOW_2D:
            texture_desc.dimension = Dimension::TEXTURE_2D;
            break;
        case AttachmentType::SHADOW_CUBE_MAP:
        case AttachmentType::COLOR_CUBE_MAP:
            texture_desc.dimension = Dimension::TEXTURE_CUBE;
            texture_desc.misc_flags |= RESOURCE_MISC_TEXTURECUBE;
            texture_desc.array_size = 6;
            break;
        default:
            CRASH_NOW();
            break;
    }
    switch (p_desc.type) {
        case AttachmentType::COLOR_2D:
        case AttachmentType::COLOR_CUBE_MAP:
            texture_desc.bind_flags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
            break;
        case AttachmentType::SHADOW_2D:
        case AttachmentType::SHADOW_CUBE_MAP:
            texture_desc.bind_flags |= BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
            break;
        case AttachmentType::DEPTH_2D:
        case AttachmentType::DEPTH_STENCIL_2D:
            texture_desc.bind_flags |= BIND_DEPTH_STENCIL;
            break;
        default:
            break;
    }

    texture_desc.name = p_desc.name;
    texture_desc.format = p_desc.format;
    texture_desc.width = p_desc.width;
    texture_desc.height = p_desc.height;
    texture_desc.initial_data = nullptr;
    texture_desc.mip_levels = 1;

    if (p_desc.need_uav) {
        texture_desc.bind_flags |= BIND_UNORDERED_ACCESS;
    }

    if (p_desc.gen_mipmap) {
        texture_desc.misc_flags |= RESOURCE_MISC_GENERATE_MIPS;
    }

    resource->texture = CreateTexture(texture_desc, p_sampler);

    m_resourceLookup[resource->desc.name] = resource;
    return resource;
}

std::shared_ptr<RenderTarget> GraphicsManager::FindRenderTarget(RenderTargetResourceName p_name) const {
    if (m_resourceLookup.empty()) {
        return nullptr;
    }

    auto it = m_resourceLookup.find(p_name);
    if (it == m_resourceLookup.end()) {
        return nullptr;
    }
    return it->second;
}

uint64_t GraphicsManager::GetFinalImage() const {
    const GpuTexture* texture = nullptr;
    switch (m_method) {
        case RenderGraph::VXGI:
            texture = FindRenderTarget(RESOURCE_FINAL)->texture.get();
            break;
        case RenderGraph::DEFAULT:
            texture = FindRenderTarget(RESOURCE_TONE)->texture.get();
            break;
        default:
            CRASH_NOW();
            break;
    }

    if (texture) {
        return texture->GetHandle();
    }

    return 0;
}

static void FillMaterialConstantBuffer(const MaterialComponent* material, MaterialConstantBuffer& cb) {
    cb.u_base_color = material->base_color;
    cb.u_metallic = material->metallic;
    cb.u_roughness = material->roughness;
    cb.u_emissive_power = material->emissive;

    auto set_texture = [&](int p_idx, sampler2D& p_out_handle) {
        p_out_handle = 0;

        if (!material->textures[p_idx].enabled) {
            return false;
        }

        ImageHandle* handle = material->textures[p_idx].image;
        if (!handle) {
            return false;
        }

        Image* image = handle->Get();
        if (!image) {
            return false;
        }

        auto texture = reinterpret_cast<GpuTexture*>(image->gpu_texture.get());
        if (!texture) {
            return false;
        }

        p_out_handle = texture->GetHandle();
        return true;
    };

    cb.u_has_base_color_map = set_texture(MaterialComponent::TEXTURE_BASE, cb.u_base_color_map_handle);
    cb.u_has_normal_map = set_texture(MaterialComponent::TEXTURE_NORMAL, cb.u_normal_map_handle);
    cb.u_has_pbr_map = set_texture(MaterialComponent::TEXTURE_METALLIC_ROUGHNESS, cb.u_material_map_handle);
}

void GraphicsManager::Cleanup() {
    m_context.batch_cache.Clear();
    m_context.material_cache.Clear();
    m_context.bone_cache.Clear();
    m_context.pass_cache.clear();

    for (auto& pass : m_shadowPasses) {
        pass.draws.clear();
    }

    for (auto& pass : m_pointShadowPasses) {
        pass.reset();
    }

    m_mainPass.draws.clear();
    m_voxelPass.draws.clear();
}

void GraphicsManager::UpdateConstants(const Scene& p_scene) {
    Camera& camera = *p_scene.m_camera.get();

    auto& cache = g_per_frame_cache.cache;
    cache.u_camera_position = camera.getPosition();

    cache.u_enable_vxgi = DVAR_GET_BOOL(r_enable_vxgi);
    cache.u_debug_voxel_id = DVAR_GET_INT(r_debug_vxgi_voxel);
    cache.u_no_texture = DVAR_GET_BOOL(r_no_texture);

    cache.u_screen_width = (int)camera.getWidth();
    cache.u_screen_height = (int)camera.getHeight();

    // Bloom
    cache.u_bloom_threshold = DVAR_GET_FLOAT(r_bloom_threshold);
    cache.u_enable_bloom = DVAR_GET_BOOL(r_enable_bloom);

    // @TODO: refactor the following
    const int voxel_texture_size = DVAR_GET_INT(r_voxel_size);
    DEV_ASSERT(math::IsPowerOfTwo(voxel_texture_size));
    DEV_ASSERT(voxel_texture_size <= 256);

    vec3 world_center = p_scene.GetBound().Center();
    vec3 aabb_size = p_scene.GetBound().Size();
    float world_size = glm::max(aabb_size.x, glm::max(aabb_size.y, aabb_size.z));

    const float max_world_size = DVAR_GET_FLOAT(r_vxgi_max_world_size);
    if (world_size > max_world_size) {
        world_center = camera.getPosition();
        world_size = max_world_size;
    }

    const float texel_size = 1.0f / static_cast<float>(voxel_texture_size);
    const float voxel_size = world_size * texel_size;

    cache.u_world_center = world_center;
    cache.u_world_size_half = 0.5f * world_size;
    cache.u_texel_size = texel_size;
    cache.u_voxel_size = voxel_size;
}

void GraphicsManager::UpdateParticles(const Scene& p_scene) {
    unused(p_scene);

    // bool should_break = true;

    // for (auto [emitter_entity, emitter_component] : p_scene.m_ParticleEmitterComponents) {
    //     m_particle_count = 0;
    //     for (const auto& particle : emitter_component.GetParticlePoolRef()) {
    //         if (particle.isActive) {
    //             if (m_particle_count >= array_length(g_particleCache.cache.globalParticleTransforms)) {
    //                 break;
    //             }
    //             g_particleCache.cache.globalParticleTransforms[m_particle_count++] = vec4(particle.position, emitter_component.GetParticleScale());
    //         }
    //     }

    //    // @TODO: only support 1 emitter
    //    if (should_break) {
    //        break;
    //    }
    //}
}

/// @TODO: refactor lights
void GraphicsManager::UpdateLights(const Scene& p_scene) {
    const uint32_t light_count = glm::min<uint32_t>((uint32_t)p_scene.GetCount<LightComponent>(), MAX_LIGHT_COUNT);

    auto& cache = g_per_frame_cache.cache;

    cache.u_light_count = light_count;

    int idx = 0;
    for (auto [light_entity, light_component] : p_scene.m_LightComponents) {
        const TransformComponent* light_transform = p_scene.GetComponent<TransformComponent>(light_entity);
        const MaterialComponent* material = p_scene.GetComponent<MaterialComponent>(light_entity);

        DEV_ASSERT(light_transform && material);

        // SHOULD BE THIS INDEX
        Light& light = cache.u_lights[idx];
        bool cast_shadow = light_component.CastShadow();
        light.cast_shadow = cast_shadow;
        light.type = light_component.GetType();
        light.color = material->base_color;
        light.color *= material->emissive;
        switch (light_component.GetType()) {
            case LIGHT_TYPE_INFINITE: {
                mat4 light_local_matrix = light_transform->GetLocalMatrix();
                vec3 light_dir = glm::normalize(light_local_matrix * vec4(0, 0, 1, 0));
                light.cast_shadow = cast_shadow;
                light.position = light_dir;

                const AABB& world_bound = p_scene.GetBound();
                const vec3 center = world_bound.Center();
                const vec3 extents = world_bound.Size();
                const float size = 0.7f * glm::max(extents.x, glm::max(extents.y, extents.z));

                vec3 light_up = glm::normalize(light_local_matrix * vec4(0, -1, 0, 0));

                light.view_matrix = glm::lookAt(center + light_dir * size, center, vec3(0, 1, 0));

                if (GetBackend() == Backend::OPENGL) {
                    light.projection_matrix = BuildOpenGLOrthoRH(-size, size, -size, size, -size, 3.0f * size);
                } else {
                    light.projection_matrix = BuildOrthoRH(-size, size, -size, size, -size, 3.0f * size);
                }

                mat4 light_space_matrix = light.projection_matrix * light.view_matrix;

                PerPassConstantBuffer pass_constant;
                // @TODO: Build correct matrices
                pass_constant.g_projection_matrix = light.projection_matrix;
                pass_constant.g_view_matrix = light.view_matrix;
                m_shadowPasses[0].pass_idx = static_cast<int>(m_context.pass_cache.size());
                m_context.pass_cache.emplace_back(pass_constant);

                // Frustum light_frustum(light_space_matrix);
                FillPass(
                    p_scene,
                    m_shadowPasses[0],
                    [](const ObjectComponent& p_object) {
                        return p_object.flags & ObjectComponent::CAST_SHADOW;
                    },
                    [&](const AABB& p_aabb) {
                        unused(p_aabb);
                        // return light_frustum.intersects(aabb);
                        return true;
                    });
            } break;
            case LIGHT_TYPE_POINT: {
                const int shadow_map_index = light_component.GetShadowMapIndex();
                // @TODO: there's a bug in shadow map allocation
                light.atten_constant = light_component.m_atten.constant;
                light.atten_linear = light_component.m_atten.linear;
                light.atten_quadratic = light_component.m_atten.quadratic;
                light.position = light_component.GetPosition();
                light.cast_shadow = cast_shadow;
                light.max_distance = light_component.GetMaxDistance();
                if (cast_shadow && shadow_map_index != INVALID_POINT_SHADOW_HANDLE) {
                    light.shadow_map_index = shadow_map_index;

                    auto pass = std::make_unique<PassContext>();
                    FillPass(
                        p_scene,
                        *pass.get(),
                        [](const ObjectComponent& object) {
                            return object.flags & ObjectComponent::CAST_SHADOW;
                        },
                        [&](const AABB& aabb) {
                            unused(aabb);
                            return true;
                        });

                    DEV_ASSERT_INDEX(shadow_map_index, MAX_LIGHT_CAST_SHADOW_COUNT);
                    pass->light_component = light_component;

                    m_pointShadowPasses[shadow_map_index] = std::move(pass);
                } else {
                    light.shadow_map_index = -1;
                }
            } break;
            case LIGHT_TYPE_AREA: {
                mat4 transform = light_transform->GetWorldMatrix();
                constexpr float s = 0.5f;
                light.points[0] = transform * vec4(-s, +s, 0.0f, 1.0f);
                light.points[1] = transform * vec4(-s, -s, 0.0f, 1.0f);
                light.points[2] = transform * vec4(+s, -s, 0.0f, 1.0f);
                light.points[3] = transform * vec4(+s, +s, 0.0f, 1.0f);
            } break;
            default:
                CRASH_NOW();
                break;
        }
        ++idx;
    }
}

void GraphicsManager::UpdateVoxelPass(const Scene& p_scene) {
    FillPass(
        p_scene,
        m_voxelPass,
        [](const ObjectComponent& object) {
            return object.flags & ObjectComponent::RENDERABLE;
        },
        [&](const AABB& aabb) {
            unused(aabb);
            // return scene->get_bound().intersects(aabb);
            return true;
        });
}

void GraphicsManager::UpdateMainPass(const Scene& p_scene) {
    const Camera& camera = *p_scene.m_camera;
    Frustum camera_frustum(camera.getProjectionViewMatrix());

    // main pass
    PerPassConstantBuffer pass_constant;
    pass_constant.g_view_matrix = camera.getViewMatrix();

    const float fovy = camera.getFovy().ToRad();
    const float aspect = camera.getAspect();
    if (GetBackend() == Backend::OPENGL) {
        pass_constant.g_projection_matrix = BuildOpenGLPerspectiveRH(fovy, aspect, camera.getNear(), camera.getFar());
    } else {
        pass_constant.g_projection_matrix = BuildPerspectiveRH(fovy, aspect, camera.getNear(), camera.getFar());
    }

    m_mainPass.pass_idx = static_cast<int>(m_context.pass_cache.size());
    m_context.pass_cache.emplace_back(pass_constant);

    FillPass(
        p_scene,
        m_mainPass,
        [](const ObjectComponent& object) {
            return object.flags & ObjectComponent::RENDERABLE;
        },
        [&](const AABB& aabb) {
            return camera_frustum.Intersects(aabb);
        });
}

void GraphicsManager::FillPass(const Scene& p_scene, PassContext& p_pass, FilterObjectFunc1 p_filter1, FilterObjectFunc2 p_filter2) {
    for (auto [entity, obj] : p_scene.m_ObjectComponents) {
        if (!p_scene.Contains<TransformComponent>(entity)) {
            continue;
        }

        if (!p_filter1(obj)) {
            continue;
        }

        const TransformComponent& transform = *p_scene.GetComponent<TransformComponent>(entity);
        DEV_ASSERT(p_scene.Contains<MeshComponent>(obj.meshId));
        const MeshComponent& mesh = *p_scene.GetComponent<MeshComponent>(obj.meshId);

        const mat4& world_matrix = transform.GetWorldMatrix();
        AABB aabb = mesh.local_bound;
        aabb.ApplyMatrix(world_matrix);
        if (!p_filter2(aabb)) {
            continue;
        }

        PerBatchConstantBuffer batch_buffer;
        batch_buffer.u_world_matrix = world_matrix;

        BatchContext draw;
        draw.flags = 0;
        if (entity == p_scene.m_selected) {
            draw.flags |= STENCIL_FLAG_SELECTED;
        }

        draw.batch_idx = m_context.batch_cache.FindOrAdd(entity, batch_buffer);
        if (mesh.armature_id.IsValid()) {
            auto& armature = *p_scene.GetComponent<ArmatureComponent>(mesh.armature_id);
            DEV_ASSERT(armature.boneTransforms.size() <= MAX_BONE_COUNT);

            BoneConstantBuffer bone;
            memcpy(bone.u_bones, armature.boneTransforms.data(), sizeof(mat4) * armature.boneTransforms.size());

            // @TODO: better memory usage
            draw.bone_idx = m_context.bone_cache.FindOrAdd(mesh.armature_id, bone);
        } else {
            draw.bone_idx = -1;
        }
        DEV_ASSERT(mesh.gpu_resource);
        draw.mesh_data = (MeshBuffers*)mesh.gpu_resource;

        for (const auto& subset : mesh.subsets) {
            aabb = subset.local_bound;
            aabb.ApplyMatrix(world_matrix);
            if (!p_filter2(aabb)) {
                continue;
            }

            const MaterialComponent* material = p_scene.GetComponent<MaterialComponent>(subset.material_id);
            MaterialConstantBuffer material_buffer;
            FillMaterialConstantBuffer(material, material_buffer);

            DrawContext sub_mesh;
            sub_mesh.index_count = subset.index_count;
            sub_mesh.index_offset = subset.index_offset;
            sub_mesh.material_idx = m_context.material_cache.FindOrAdd(subset.material_id, material_buffer);

            draw.subsets.emplace_back(std::move(sub_mesh));
        }

        p_pass.draws.emplace_back(std::move(draw));
    }
}

}  // namespace my
