#include "render_manager.h"

#include "core/debugger/profiler.h"
#include "core/framework/asset_manager.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/render_graph_defines.h"

#define DEFINE_DVAR
#include "rendering_dvars.h"

namespace {
std::string s_prev_env_map;
bool s_need_update_env = false;
}  // namespace

namespace my::renderer {

bool need_update_env() {
    return s_need_update_env;
}

void reset_need_update_env() {
    s_need_update_env = false;
}

void request_env_map(const std::string& path) {
    if (path == s_prev_env_map) {
        return;
    }

    if (!s_prev_env_map.empty()) {
        LOG_WARN("TODO: release {}", s_prev_env_map.empty());
    }

    s_prev_env_map = path;
    if (auto handle = AssetManager::singleton().find_image(path); handle) {
        if (auto image = handle->get(); image && image->gpu_texture) {
            g_constantCache.cache.c_hdr_env_map = image->gpu_texture->get_resident_handle();
            g_constantCache.update();
            s_need_update_env = true;
            return;
        }
    }

    AssetManager::singleton().load_image_async(path, [](void* p_asset, void* p_userdata) {
        Image* image = reinterpret_cast<Image*>(p_asset);
        ImageHandle* handle = reinterpret_cast<ImageHandle*>(p_userdata);
        DEV_ASSERT(image);
        DEV_ASSERT(handle);

        handle->set(image);
        GraphicsManager::singleton().request_texture(handle, [](Image* p_image) {
            // @TODO: better way
            g_constantCache.cache.c_hdr_env_map = p_image->gpu_texture->get_resident_handle();
            g_constantCache.update();
            s_need_update_env = true;
        });
    });
}

mat4 light_space_matrix_world(const AABB& p_world_bound, const mat4& p_light_matrix) {
    const vec3 center = p_world_bound.center();
    const vec3 extents = p_world_bound.size();
    const float size = 0.5f * glm::max(extents.x, glm::max(extents.y, extents.z));

    vec3 light_dir = glm::normalize(p_light_matrix * vec4(0, 0, 1, 0));
    vec3 light_up = glm::normalize(p_light_matrix * vec4(0, -1, 0, 0));

    const mat4 V = glm::lookAt(center + light_dir * size, center, vec3(0, 1, 0));
    const mat4 P = glm::ortho(-size, size, -size, size, -size, 3.0f * size);
    return P * V;
}

struct Sphere {
    vec3 center;
    float radius;
};

static Sphere find_bounding_sphere(const vec3* p_points, int p_point_count) {
    DEV_ASSERT(p_point_count >= 2);
    vec3 center{ 0 };
    float max_distance = 0.0f;
    for (int i = 0; i < p_point_count; ++i) {
        center += p_points[i];
    }
    center *= 1.0f / p_point_count;

    for (int i = 0; i < p_point_count - 1; ++i) {
        for (int j = i + 1; j < p_point_count; ++j) {
            float distance = glm::distance(p_points[i], p_points[j]);
            if (distance > max_distance) {
                max_distance = distance;
            }
        }
    }

    DEV_ASSERT(max_distance > 0.0f);
    Sphere sphere;
    sphere.center = center;
    sphere.radius = 0.5f * max_distance;
    return sphere;
}

static mat4 get_light_space_matrix(const mat4& p_light_matrix, float p_near_plane, float p_far_plane, const Camera& p_camera) {
    const auto proj = glm::perspective(
        p_camera.get_fovy().to_rad(),
        p_camera.get_aspect(),
        p_near_plane,
        p_far_plane);

    std::array<vec3, 8> corners{
        vec3(-1, -1, -1),
        vec3(-1, -1, +1),
        vec3(-1, +1, -1),
        vec3(-1, +1, +1),

        vec3(+1, -1, -1),
        vec3(+1, -1, +1),
        vec3(+1, +1, -1),
        vec3(+1, +1, +1),
    };

    mat4 inv_pv = glm::inverse(proj * p_camera.get_view_matrix());
    for (vec3& point : corners) {
        vec4 point4{ point, 1.0f };
        point4 = inv_pv * point4;
        point4 /= point4.w;  // world space
        point = point4;
    }

    Sphere sphere = find_bounding_sphere(corners.data(), static_cast<int>(corners.size()));
    float r = sphere.radius;

    vec3 light_dir = glm::normalize(p_light_matrix * vec4(0, 0, 1, 0));
    vec3 light_up = glm::normalize(p_light_matrix * vec4(0, 1, 0, 0));
    vec3 eye = sphere.center + r * light_dir;
    mat4 light_view = glm::lookAt(eye, sphere.center, light_up);

    float min_z = -3.0f * r;
    float max_z = 3.0f * r;

    mat4 light_projection = glm::ortho(-r, r, -r, r, min_z, max_z);
    return light_projection * light_view;
}

std::vector<mat4> get_light_space_matrices(const mat4& p_light_matrix, const Camera& p_camera, const vec4& p_cascade_end, const AABB& world_bound) {
    std::vector<glm::mat4> ret;
    for (int i = 0; i < MAX_CASCADE_COUNT; ++i) {
        if (!DVAR_GET_BOOL(r_enable_csm)) {
            ret.push_back(light_space_matrix_world(world_bound, p_light_matrix));
        } else {
            float z_near = p_camera.get_near();
            z_near = i == 0 ? z_near : p_cascade_end[i - 1];
            ret.push_back(get_light_space_matrix(p_light_matrix, z_near, p_cascade_end[i], p_camera));
        }
    }
    return ret;
}

// @TODO: fix this?
void fill_constant_buffers(const Scene& scene) {
    Camera& camera = *scene.m_camera.get();

    // THESE SHOULDN'T BE HERE
    auto& cache = g_perFrameCache.cache;
    const uint32_t light_count = glm::min<uint32_t>((uint32_t)scene.get_count<LightComponent>(), MAX_LIGHT_COUNT);
    // DEV_ASSERT(light_count);

    cache.u_light_count = light_count;

    // auto camera = scene.m_camera;
    vec4 cascade_end = DVAR_GET_VEC4(cascade_end);
    std::vector<mat4> light_matrices;
    if (camera.get_far() != cascade_end.w) {
        camera.set_far(cascade_end.w);
        camera.set_dirty();
    }
    for (int idx = 0; idx < MAX_CASCADE_COUNT; ++idx) {
        float left = idx == 0 ? camera.get_near() : cascade_end[idx - 1];
        DEV_ASSERT(left < cascade_end[idx]);
    }

    int idx = 0;
    for (auto [light_entity, light_component] : scene.m_LightComponents) {
        const TransformComponent* light_transform = scene.get_component<TransformComponent>(light_entity);
        const MaterialComponent* material = scene.get_component<MaterialComponent>(light_entity);

        DEV_ASSERT(light_transform && material);

        // SHOULD BE THIS INDEX
        Light& light = cache.u_lights[idx];
        bool cast_shadow = light_component.cast_shadow();
        light.cast_shadow = cast_shadow;
        light.type = light_component.get_type();
        light.color = material->base_color;
        light.color *= material->emissive;
        switch (light_component.get_type()) {
            case LIGHT_TYPE_OMNI: {
                mat4 light_matrix = light_transform->get_local_matrix();
                vec3 light_dir = glm::normalize(light_matrix * vec4(0, 0, 1, 0));
                light.cast_shadow = cast_shadow;
                light.position = light_dir;

                light_matrices = get_light_space_matrices(light_matrix, camera, cascade_end, scene.get_bound());
            } break;
            case LIGHT_TYPE_POINT: {
                const int shadow_map_index = light_component.get_shadow_map_index();
                // @TODO: there's a bug in shadow map allocation
                light.atten_constant = light_component.m_atten.constant;
                light.atten_linear = light_component.m_atten.linear;
                light.atten_quadratic = light_component.m_atten.quadratic;
                light.position = light_component.get_position();
                light.cast_shadow = cast_shadow;
                light.max_distance = light_component.get_max_distance();
                if (cast_shadow && shadow_map_index != -1) {
                    const auto& point_light_matrices = light_component.get_matrices();
                    for (size_t i = 0; i < 6; ++i) {
                        light.matrices[i] = point_light_matrices[i];
                    }
                    auto resource = GraphicsManager::singleton().find_render_target(RT_RES_POINT_SHADOW_MAP + std::to_string(shadow_map_index));
                    light.shadow_map = resource ? resource->texture->get_resident_handle() : 0;
                } else {
                    light.shadow_map = 0;
                }
            } break;
            case LIGHT_TYPE_AREA: {
                mat4 transform = light_transform->get_world_matrix();
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

    // @TODO: fix this
    if (!light_matrices.empty()) {
        for (int idx2 = 0; idx2 < MAX_CASCADE_COUNT; ++idx2) {
            cache.u_main_light_matrices[idx2] = light_matrices[idx2];
        }
    }

    cache.u_cascade_plane_distances = cascade_end;

    cache.u_camera_position = camera.get_position();

    cache.u_enable_vxgi = DVAR_GET_BOOL(r_enable_vxgi);
    cache.u_debug_voxel_id = DVAR_GET_INT(r_debug_vxgi_voxel);
    cache.u_no_texture = DVAR_GET_BOOL(r_no_texture);
    cache.u_debug_csm = DVAR_GET_BOOL(r_debug_csm);
    cache.u_enable_csm = DVAR_GET_BOOL(r_enable_csm);

    cache.u_screen_width = (int)camera.get_width();
    cache.u_screen_height = (int)camera.get_height();

    // SSAO
    cache.u_ssao_kernel_size = DVAR_GET_INT(r_ssaoKernelSize);
    cache.u_ssao_kernel_radius = DVAR_GET_FLOAT(r_ssao_kernel_radius);
    cache.u_ssao_noise_size = DVAR_GET_INT(r_ssaoNoiseSize);
    cache.u_enable_ssao = DVAR_GET_BOOL(r_enable_ssao);
    cache.u_enable_bloom = DVAR_GET_BOOL(r_enable_bloom);

    // Bloom
    cache.u_bloom_threshold = DVAR_GET_FLOAT(r_bloom_threshold);

    // @TODO: refactor the following
    const int voxel_texture_size = DVAR_GET_INT(r_voxel_size);
    DEV_ASSERT(math::is_power_of_two(voxel_texture_size));
    DEV_ASSERT(voxel_texture_size <= 256);

    vec3 world_center = scene.get_bound().center();
    vec3 aabb_size = scene.get_bound().size();
    float world_size = glm::max(aabb_size.x, glm::max(aabb_size.y, aabb_size.z));

    const float max_world_size = DVAR_GET_FLOAT(r_vxgi_max_world_size);
    if (world_size > max_world_size) {
        world_center = camera.get_position();
        world_size = max_world_size;
    }

    const float texel_size = 1.0f / static_cast<float>(voxel_texture_size);
    const float voxel_size = world_size * texel_size;

    cache.u_world_center = world_center;
    cache.u_world_size_half = 0.5f * world_size;
    cache.u_texel_size = texel_size;
    cache.u_voxel_size = voxel_size;
}

void register_rendering_dvars(){
#define REGISTER_DVAR
#include "rendering_dvars.h"
}

std::array<mat4, 6> cube_map_view_matrices(const vec3& p_eye) {
    std::array<mat4, 6> matrices;
    matrices[0] = glm::lookAt(p_eye, p_eye + glm::vec3(+1, +0, +0), glm::vec3(0, -1, +0));
    matrices[1] = glm::lookAt(p_eye, p_eye + glm::vec3(-1, +0, +0), glm::vec3(0, -1, +0));
    matrices[2] = glm::lookAt(p_eye, p_eye + glm::vec3(+0, +1, +0), glm::vec3(0, +0, +1));
    matrices[3] = glm::lookAt(p_eye, p_eye + glm::vec3(+0, -1, +0), glm::vec3(0, +0, -1));
    matrices[4] = glm::lookAt(p_eye, p_eye + glm::vec3(+0, +0, +1), glm::vec3(0, -1, +0));
    matrices[5] = glm::lookAt(p_eye, p_eye + glm::vec3(+0, +0, -1), glm::vec3(0, -1, +0));
    return matrices;
}

void fill_texture_and_sampler_desc(const Image* p_image, TextureDesc& p_texture_desc, SamplerDesc& p_sampler_desc) {
    DEV_ASSERT(p_image);
    bool is_hdr_file = false;

    switch (p_image->format) {
        case PixelFormat::R32_FLOAT:
        case PixelFormat::R32G32_FLOAT:
        case PixelFormat::R32G32B32_FLOAT:
        case PixelFormat::R32G32B32A32_FLOAT: {
            is_hdr_file = true;
        } break;
        default: {
        } break;
    }

    p_texture_desc.format = p_image->format;
    p_texture_desc.dimension = Dimension::TEXTURE_2D;
    p_texture_desc.width = p_image->width;
    p_texture_desc.height = p_image->height;
    p_texture_desc.array_size = 1;
    p_texture_desc.bind_flags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    p_texture_desc.initial_data = p_image->buffer.data();
    p_texture_desc.mip_levels = 1;

    if (is_hdr_file) {
        p_sampler_desc.min = p_sampler_desc.mag = FilterMode::LINEAR;
        p_sampler_desc.mode_u = p_sampler_desc.mode_v = AddressMode::CLAMP;
    } else {
        p_texture_desc.misc_flags |= RESOURCE_MISC_GENERATE_MIPS;

        p_sampler_desc.min = FilterMode::MIPMAP_LINEAR;
        p_sampler_desc.mag = FilterMode::LINEAR;
    }
}

}  // namespace my::renderer

namespace my {

RenderManager::RenderManager() : Module("RenderManager") {
    for (int i = 0; i < MAX_LIGHT_CAST_SHADOW_COUNT; ++i) {
        m_free_point_light_shadow.push_back(i);
    }
}

bool RenderManager::initialize() {
    return true;
}

void RenderManager::finalize() {
    m_free_point_light_shadow.clear();
    return;
}

void RenderManager::update(Scene& p_scene) {
    OPTICK_EVENT();

    renderer::fill_constant_buffers(p_scene);
    GraphicsManager::singleton().get_render_data()->update(&p_scene);
}

PointShadowHandle RenderManager::allocate_point_light_shadow_map() {
    if (m_free_point_light_shadow.empty()) {
        LOG_WARN("OUT OUT POINT SHADOW MAP");
        return INVALID_POINT_SHADOW_HANDLE;
    }

    int handle = m_free_point_light_shadow.front();
    m_free_point_light_shadow.pop_front();
    return handle;
}

void RenderManager::free_point_light_shadow_map(PointShadowHandle& p_handle) {
    DEV_ASSERT_INDEX(p_handle, MAX_LIGHT_CAST_SHADOW_COUNT);
    m_free_point_light_shadow.push_back(p_handle);
    p_handle = INVALID_POINT_SHADOW_HANDLE;
}

}  // namespace my
