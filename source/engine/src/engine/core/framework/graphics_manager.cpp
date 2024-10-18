#include "graphics_manager.h"

#include "core/debugger/profiler.h"
#include "core/math/frustum.h"
#include "drivers/d3d11/d3d11_graphics_manager.h"
#include "drivers/empty/empty_graphics_manager.h"
#include "drivers/opengl/opengl_graphics_manager.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"

namespace my {

template<typename T>
static auto create_uniform(GraphicsManager& p_graphics_manager, uint32_t p_max_count) {
    static_assert(sizeof(T) % 256 == 0);
    return p_graphics_manager.createUniform(T::get_uniform_buffer_slot(), sizeof(T) * p_max_count);
}

UniformBuffer<PerPassConstantBuffer> g_per_pass_cache;
UniformBuffer<PerFrameConstantBuffer> g_per_frame_cache;
UniformBuffer<PerSceneConstantBuffer> g_constantCache;
UniformBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;

template<typename T>
static void create_uniform_buffer(UniformBuffer<T>& p_buffer) {
    constexpr int slot = T::get_uniform_buffer_slot();
    p_buffer.buffer = GraphicsManager::singleton().createUniform(slot, sizeof(T));
}

bool GraphicsManager::initialize() {
    if (!initializeImpl()) {
        return false;
    }

    // @TODO: refactor
    m_context.batch_uniform = create_uniform<PerBatchConstantBuffer>(*this, 4096 * 16);
    m_context.material_uniform = create_uniform<MaterialConstantBuffer>(*this, 2048 * 16);
    m_context.bone_uniform = create_uniform<BoneConstantBuffer>(*this, 16);

    create_uniform_buffer<PerPassConstantBuffer>(g_per_pass_cache);
    create_uniform_buffer<PerFrameConstantBuffer>(g_per_frame_cache);
    create_uniform_buffer<PerSceneConstantBuffer>(g_constantCache);
    create_uniform_buffer<DebugDrawConstantBuffer>(g_debug_draw_cache);

    DEV_ASSERT(m_pipeline_state_manager);

    if (!m_pipeline_state_manager->initialize()) {
        return false;
    }

    auto bind_slot = [&](const std::string& name, int slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = findRenderTarget(name);
        if (!resource) {
            return;
        }

        bindTexture(p_dimension, resource->texture->get_handle(), slot);
    };

    // bind common textures
    bind_slot(RT_RES_SHADOW_MAP, u_shadow_map_slot);
    bind_slot(RT_RES_HIGHLIGHT_SELECT, u_selection_highlight_slot);
    bind_slot(RT_RES_GBUFFER_BASE_COLOR, u_gbuffer_base_color_map_slot);
    bind_slot(RT_RES_GBUFFER_POSITION, u_gbuffer_position_map_slot);
    bind_slot(RT_RES_GBUFFER_NORMAL, u_gbuffer_normal_map_slot);
    bind_slot(RT_RES_GBUFFER_MATERIAL, u_gbuffer_material_map_slot);

    return true;
}

void GraphicsManager::eventReceived(std::shared_ptr<Event> event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(event.get()); e) {
        onSceneChange(*e->getScene());
    }
    if (ResizeEvent* e = dynamic_cast<ResizeEvent*>(event.get()); e) {
        onWindowResize(e->getWidth(), e->getHeight());
    }
}

std::shared_ptr<GraphicsManager> GraphicsManager::create() {
    const std::string& backend = DVAR_GET_STRING(r_backend);

    if (backend == "opengl") {
        return std::make_shared<OpenGLGraphicsManager>();
    } else if (backend == "d3d11") {
        return std::make_shared<D3d11GraphicsManager>();
    }
    return std::make_shared<EmptyGraphicsManager>(Backend::EMPTY);
}

void GraphicsManager::setPipelineState(PipelineStateName p_name) {
    if (m_last_pipeline_name != p_name) {
        setPipelineStateImpl(p_name);
        m_last_pipeline_name = p_name;
    }
}

void GraphicsManager::requestTexture(ImageHandle* p_handle, OnTextureLoadFunc p_func) {
    m_loaded_images.push(ImageTask{ p_handle, p_func });
}

void GraphicsManager::update(Scene& p_scene) {
    OPTICK_EVENT();

    cleanup();

    updatePointLights(p_scene);
    updateOmniLights(p_scene);
    updateVoxelPass(p_scene);
    updateMainPass(p_scene);

    // update uniform
    updateUniform(m_context.batch_uniform.get(), m_context.batch_cache.buffer);
    updateUniform(m_context.material_uniform.get(), m_context.material_cache.buffer);
    updateUniform(m_context.bone_uniform.get(), m_context.bone_cache.buffer);

    g_per_frame_cache.update();
    // update uniform

    // @TODO: make it a function
    auto loaded_images = m_loaded_images.pop_all();
    while (!loaded_images.empty()) {
        auto task = loaded_images.front();
        loaded_images.pop();
        DEV_ASSERT(task.handle->state == ASSET_STATE_READY);
        Image* image = task.handle->get();
        DEV_ASSERT(image);

        TextureDesc texture_desc{};
        SamplerDesc sampler_desc{};
        renderer::fill_texture_and_sampler_desc(image, texture_desc, sampler_desc);

        image->gpu_texture = createTexture(texture_desc, sampler_desc);
        if (task.func) {
            task.func(task.handle->get());
        }
    }

    render();
}

void GraphicsManager::selectRenderGraph() {
    std::string method(DVAR_GET_STRING(r_render_graph));
    if (method == "vxgi") {
        m_method = RenderGraph::VXGI;
    } else {
        m_method = RenderGraph::DEFAULT;
    }

    if (m_backend == Backend::D3D11) {
        m_method = RenderGraph::DEFAULT;
    }

    switch (m_method) {
        case RenderGraph::VXGI:
            createRenderGraphVxgi(m_render_graph);
            break;
        default:
            createRenderGraphDefault(m_render_graph);
            break;
    }
}

std::shared_ptr<RenderTarget> GraphicsManager::createRenderTarget(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler) {
    DEV_ASSERT(m_resource_lookup.find(p_desc.name) == m_resource_lookup.end());
    std::shared_ptr<RenderTarget> resource = std::make_shared<RenderTarget>(p_desc);

    // @TODO: this part need rework
    TextureDesc texture_desc{};
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

    texture_desc.format = p_desc.format;
    texture_desc.width = p_desc.width;
    texture_desc.height = p_desc.height;
    texture_desc.initial_data = nullptr;
    texture_desc.misc_flags = 0;
    texture_desc.mip_levels = 1;
    texture_desc.array_size = 1;
    if (p_desc.gen_mipmap) {
        texture_desc.misc_flags |= RESOURCE_MISC_GENERATE_MIPS;
    }

    resource->texture = createTexture(texture_desc, p_sampler);

    m_resource_lookup[resource->desc.name] = resource;
    return resource;
}

std::shared_ptr<RenderTarget> GraphicsManager::findRenderTarget(const std::string& name) const {
    if (m_resource_lookup.empty()) {
        return nullptr;
    }

    auto it = m_resource_lookup.find(name);
    if (it == m_resource_lookup.end()) {
        return nullptr;
    }
    return it->second;
}

uint64_t GraphicsManager::getFinalImage() const {
    const Texture* texture = nullptr;
    switch (m_method) {
        case RenderGraph::VXGI:
            texture = findRenderTarget(RT_RES_FINAL)->texture.get();
            break;
        case RenderGraph::DEFAULT:
            texture = findRenderTarget(RT_RES_LIGHTING)->texture.get();
            break;
        default:
            CRASH_NOW();
            break;
    }

    if (texture) {
        return texture->get_handle();
    }

    return 0;
}

// @TODO: refactor
void PassContext::fillPerpass(PerPassConstantBuffer& buffer) const {
    static const mat4 fixup = mat4({ 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 0.5, 0 }, { 0, 0, 0, 1 }) * mat4({ 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 1, 1 });

    buffer.u_view_matrix = view_matrix;

    if (GraphicsManager::singleton().getBackend() == Backend::D3D11) {
        buffer.u_proj_matrix = fixup * projection_matrix;
        buffer.u_proj_view_matrix = fixup * projection_view_matrix;
    } else {
        buffer.u_proj_matrix = projection_matrix;
        buffer.u_proj_view_matrix = projection_view_matrix;
    }
}

static void fill_material_constant_buffer(const MaterialComponent* material, MaterialConstantBuffer& cb) {
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

        Image* image = handle->get();
        if (!image) {
            return false;
        }

        auto texture = reinterpret_cast<Texture*>(image->gpu_texture.get());
        if (!texture) {
            return false;
        }

        p_out_handle = texture->get_handle();
        return true;
    };

    cb.u_has_base_color_map = set_texture(MaterialComponent::TEXTURE_BASE, cb.u_base_color_map_handle);
    cb.u_has_normal_map = set_texture(MaterialComponent::TEXTURE_NORMAL, cb.u_normal_map_handle);
    cb.u_has_pbr_map = set_texture(MaterialComponent::TEXTURE_METALLIC_ROUGHNESS, cb.u_material_map_handle);
}

void GraphicsManager::cleanup() {
    m_context.batch_cache.newFrame();
    m_context.material_cache.newFrame();
    m_context.bone_cache.newFrame();

    for (auto& pass : shadow_passes) {
        pass.clear();
    }

    for (auto& pass : point_shadow_passes) {
        pass.reset();
    }

    main_pass.clear();
    voxel_pass.clear();
}

void GraphicsManager::updatePointLights(const Scene& p_scene) {
    for (auto [light_id, light] : p_scene.m_LightComponents) {
        if (light.getType() != LIGHT_TYPE_POINT || !light.castShadow()) {
            continue;
        }

        const int light_pass_index = light.getShadowMapIndex();
        if (light_pass_index == INVALID_POINT_SHADOW_HANDLE) {
            continue;
        }

        const TransformComponent* transform = p_scene.getComponent<TransformComponent>(light_id);
        DEV_ASSERT(transform);
        vec3 position = transform->getTranslation();

        const auto& light_space_matrices = light.getMatrices();
        std::array<Frustum, 6> frustums = {
            Frustum{ light_space_matrices[0] },
            Frustum{ light_space_matrices[1] },
            Frustum{ light_space_matrices[2] },
            Frustum{ light_space_matrices[3] },
            Frustum{ light_space_matrices[4] },
            Frustum{ light_space_matrices[5] },
        };

        auto pass = std::make_unique<PassContext>();
        fillPass(
            p_scene,
            *pass.get(),
            [](const ObjectComponent& object) {
                return object.flags & ObjectComponent::CAST_SHADOW;
            },
            [&](const AABB& aabb) {
                for (const auto& frustum : frustums) {
                    if (frustum.intersects(aabb)) {
                        return true;
                    }
                }
                return false;
            });

        DEV_ASSERT_INDEX(light_pass_index, MAX_LIGHT_CAST_SHADOW_COUNT);
        pass->light_component = light;

        point_shadow_passes[light_pass_index] = std::move(pass);
    }
}

void GraphicsManager::updateOmniLights(const Scene& p_scene) {
    // @TODO: remove this
    has_sun_light = false;
    for (auto [entity, light] : p_scene.m_LightComponents) {
        if (light.getType() == LIGHT_TYPE_OMNI) {
            has_sun_light = true;
            break;
        }
    }

    if (!has_sun_light) {
        return;
    }

    // cascaded shadow map
    for (int i = 0; i < MAX_CASCADE_COUNT; ++i) {
        // @TODO: fix this
        mat4 light_matrix = g_per_frame_cache.cache.u_main_light_matrices[i];
        Frustum light_frustum(light_matrix);
        static const mat4 fixup = mat4({ 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 0.5, 0 }, { 0, 0, 0, 1 }) * mat4({ 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 1, 1 });

        if (GraphicsManager::singleton().getBackend() == Backend::D3D11) {
            light_matrix = fixup * light_matrix;
        }

        shadow_passes[i].projection_view_matrix = light_matrix;
        fillPass(
            p_scene,
            shadow_passes[i],
            [](const ObjectComponent& object) {
                return object.flags & ObjectComponent::CAST_SHADOW;
            },
            [&](const AABB& aabb) {
                return light_frustum.intersects(aabb);
            });
    }
}

void GraphicsManager::updateVoxelPass(const Scene& p_scene) {
    fillPass(
        p_scene,
        voxel_pass,
        [](const ObjectComponent& object) {
            return object.flags & ObjectComponent::RENDERABLE;
        },
        [&](const AABB& aabb) {
            unused(aabb);
            // return scene->get_bound().intersects(aabb);
            return true;
        });
}

void GraphicsManager::updateMainPass(const Scene& p_scene) {
    mat4 camera_projection_view = p_scene.m_camera->getProjectionViewMatrix();
    Frustum camera_frustum(camera_projection_view);
    // main pass
    main_pass.projection_matrix = p_scene.m_camera->getProjectionMatrix();
    main_pass.view_matrix = p_scene.m_camera->getViewMatrix();
    main_pass.projection_view_matrix = p_scene.m_camera->getProjectionViewMatrix();
    fillPass(
        p_scene,
        main_pass,
        [](const ObjectComponent& object) {
            return object.flags & ObjectComponent::RENDERABLE;
        },
        [&](const AABB& aabb) {
            return camera_frustum.intersects(aabb);
        });
}

void GraphicsManager::fillPass(const Scene& p_scene, PassContext& p_pass, FilterObjectFunc1 p_filter1, FilterObjectFunc2 p_filter2) {
    for (auto [entity, obj] : p_scene.m_ObjectComponents) {
        if (!p_scene.contains<TransformComponent>(entity)) {
            continue;
        }

        if (!p_filter1(obj)) {
            continue;
        }

        const TransformComponent& transform = *p_scene.getComponent<TransformComponent>(entity);
        DEV_ASSERT(p_scene.contains<MeshComponent>(obj.mesh_id));
        const MeshComponent& mesh = *p_scene.getComponent<MeshComponent>(obj.mesh_id);

        const mat4& world_matrix = transform.getWorldMatrix();
        AABB aabb = mesh.local_bound;
        aabb.applyMatrix(world_matrix);
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

        draw.batch_idx = m_context.batch_cache.findOrAdd(entity, batch_buffer);
        if (mesh.armature_id.isValid()) {
            auto& armature = *p_scene.getComponent<ArmatureComponent>(mesh.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            BoneConstantBuffer bone;
            memcpy(bone.u_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());

            // @TODO: better memory usage
            draw.bone_idx = m_context.bone_cache.findOrAdd(mesh.armature_id, bone);
        } else {
            draw.bone_idx = -1;
        }
        DEV_ASSERT(mesh.gpu_resource);
        draw.mesh_data = (MeshBuffers*)mesh.gpu_resource;

        for (const auto& subset : mesh.subsets) {
            aabb = subset.local_bound;
            aabb.applyMatrix(world_matrix);
            if (!p_filter2(aabb)) {
                continue;
            }

            const MaterialComponent* material = p_scene.getComponent<MaterialComponent>(subset.material_id);
            MaterialConstantBuffer material_buffer;
            fill_material_constant_buffer(material, material_buffer);

            DrawContext sub_mesh;
            sub_mesh.index_count = subset.index_count;
            sub_mesh.index_offset = subset.index_offset;
            sub_mesh.material_idx = m_context.material_cache.findOrAdd(subset.material_id, material_buffer);

            draw.subsets.emplace_back(std::move(sub_mesh));
        }

        p_pass.draws.emplace_back(std::move(draw));
    }
}

}  // namespace my
