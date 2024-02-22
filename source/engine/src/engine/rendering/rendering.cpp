#include "rendering.h"

#include "core/framework/graphics_manager.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_graph/render_graph_vxgi.h"
#include "rendering/rendering_dvars.h"

namespace my {

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

static mat4 get_light_space_matrix(const mat4& p_light_matrix, float p_near_plane, float p_far_plane, const Camera& p_camera) {
    const auto proj = glm::perspective(
        p_camera.get_fovy().to_rad(),
        p_camera.get_aspect(),
        p_near_plane,
        p_far_plane);

    std::array<vec4, 8> corners{
        vec4(-1, -1, -1, 1),
        vec4(-1, -1, +1, 1),
        vec4(-1, +1, -1, 1),
        vec4(-1, +1, +1, 1),

        vec4(+1, -1, -1, 1),
        vec4(+1, -1, +1, 1),
        vec4(+1, +1, -1, 1),
        vec4(+1, +1, +1, 1),
    };

    glm::vec3 center = glm::vec3(0);
    mat4 inv_pv = glm::inverse(proj * p_camera.get_view_matrix());
    for (vec4& point : corners) {
        point = inv_pv * point;
        point /= point.w;  // world space
        center += vec3(point);
    }

    center /= corners.size();

    vec3 light_dir = glm::normalize(p_light_matrix * vec4(0, 0, 1, 0));
    vec3 light_up = glm::normalize(p_light_matrix * vec4(0, -1, 0, 0));
    vec3 eye = center + light_dir;
    mat4 light_view = glm::lookAt(eye, center, light_up);

    AABB aabb;
    for (const vec4& point : corners) {
        aabb.expand_point(vec3(light_view * point));
    }

    vec3 aabb_center = aabb.center();

    float min_x = aabb.get_min().x;
    float max_x = aabb.get_max().x;
    float min_y = aabb.get_min().y;
    float max_y = aabb.get_max().y;
    float min_z = aabb.get_min().z;
    float max_z = aabb.get_max().z;

    // HACK: extend min and max z
    // use a more math way to do it
    float dz = 2.f * (max_z - min_z);
    min_z -= dz;
    max_z += dz;

    mat4 light_projection = glm::ortho(min_x, max_x, min_y, max_y, min_z, max_z);
    return light_projection * light_view;
}

std::vector<mat4> get_light_space_matrices(const mat4& p_light_matrix, const Camera& p_camera, const vec4& p_cascade_end, const AABB& world_bound) {
    std::vector<glm::mat4> ret;
    for (int i = 0; i < MAX_CASCADE_COUNT; ++i) {
        if (!DVAR_GET_BOOL(r_enable_csm)) {
            ret.push_back(light_space_matrix_world(world_bound, p_light_matrix));
        } else {
            float z_near = p_camera.get_near();
            // z_near = i == 0 ? z_near : p_cascade_end[i - 1];
            ret.push_back(get_light_space_matrix(p_light_matrix, z_near, p_cascade_end[i], p_camera));
        }
    }
    return ret;
}

void fill_constant_buffers(const Scene& scene) {
    Camera& camera = *scene.m_camera.get();

    // THESE SHOULDN'T BE HERE
    auto& cache = g_perFrameCache.cache;
    const uint32_t light_count = glm::min<uint32_t>((uint32_t)scene.get_count<LightComponent>(), MAX_LIGHT_COUNT);
    // DEV_ASSERT(light_count);

    cache.c_light_count = light_count;

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

    int num_point_light_cast_shadow = 0;
    for (uint32_t idx = 0; idx < light_count; ++idx) {
        const LightComponent& light_component = scene.get_component_array<LightComponent>()[idx];
        auto light_entity = scene.get_entity<LightComponent>(idx);
        const TransformComponent* light_transform = scene.get_component<TransformComponent>(light_entity);
        DEV_ASSERT(light_transform);

        Light& light = cache.c_lights[idx];
        light.cast_shadow = false;
        light.type = light_component.type;
        light.color = light_component.color * light_component.energy;
        switch (light_component.type) {
            case LIGHT_TYPE_OMNI: {
                mat4 light_matrix = light_transform->get_local_matrix();
                vec3 light_dir = glm::normalize(light_matrix * vec4(0, 0, 1, 0));
                light.cast_shadow = light_component.cast_shadow();
                light.position = light_dir;

                light_matrices = get_light_space_matrices(light_matrix, camera, cascade_end, scene.get_bound());
            } break;
            case LIGHT_TYPE_POINT: {
                // @TODO: there's a bug in shadow map allocation
                light.atten_constant = light_component.atten.constant;
                light.atten_linear = light_component.atten.linear;
                light.atten_quadratic = light_component.atten.quadratic;
                const vec3 position = light_transform->get_translation();
                light.position = position;
                light.cast_shadow = light_component.cast_shadow();
                constexpr float near_plane = LIGHT_SHADOW_MIN_DISTANCE;
                light.max_distance = light_component.max_distance;
                if (light.cast_shadow) {
                    CRASH_COND_MSG(num_point_light_cast_shadow >= MAX_LIGHT_CAST_SHADOW_COUNT, "Can have at most " _STR(MAX_LIGHT_CAST_SHADOW_COUNT) " point lights that cast shadow");

                    const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, light.max_distance);
                    light.matrices[0] = projection * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
                    light.matrices[1] = projection * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
                    light.matrices[2] = projection * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                    light.matrices[3] = projection * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
                    light.matrices[4] = projection * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
                    light.matrices[5] = projection * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

                    // @TODO: allocate
                    auto resource = GraphicsManager::singleton().find_resource(RT_RES_POINT_SHADOW_MAP + std::to_string(num_point_light_cast_shadow));
                    light.shadow_map.data = resource ? resource->get_resident_handle() : 0;

                    ++num_point_light_cast_shadow;
                }
            } break;
            default:
                break;
        }
    }

    if (!light_matrices.empty()) {
        for (int idx = 0; idx < MAX_CASCADE_COUNT; ++idx) {
            cache.c_main_light_matrices[idx] = light_matrices[idx];
        }
    }

    cache.c_cascade_plane_distances = cascade_end;

    cache.c_camera_position = camera.get_position();
    cache.c_view_matrix = camera.get_view_matrix();
    cache.c_projection_matrix = camera.get_projection_matrix();
    cache.c_projection_view_matrix = camera.get_projection_view_matrix();

    cache.c_enable_vxgi = DVAR_GET_BOOL(r_enable_vxgi);
    cache.c_debug_voxel_id = DVAR_GET_INT(r_debug_vxgi_voxel);
    cache.c_no_texture = DVAR_GET_BOOL(r_no_texture);
    cache.c_debug_csm = DVAR_GET_BOOL(r_debug_csm);
    cache.c_enable_csm = DVAR_GET_BOOL(r_enable_csm);

    cache.c_screen_width = (int)camera.get_width();
    cache.c_screen_height = (int)camera.get_height();

    // SSAO
    cache.c_ssao_kernel_size = DVAR_GET_INT(r_ssaoKernelSize);
    cache.c_ssao_kernel_radius = DVAR_GET_FLOAT(r_ssaoKernelRadius);
    cache.c_ssao_noise_size = DVAR_GET_INT(r_ssaoNoiseSize);
    cache.c_enable_ssao = DVAR_GET_BOOL(r_enable_ssao);

    // c_fxaa_image
    cache.c_enable_fxaa = DVAR_GET_BOOL(r_enable_fxaa);

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

    cache.c_world_center = world_center;
    cache.c_world_size_half = 0.5f * world_size;
    cache.c_texel_size = texel_size;
    cache.c_voxel_size = voxel_size;
}

}  // namespace my