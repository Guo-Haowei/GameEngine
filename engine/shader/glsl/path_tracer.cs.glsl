/// File: path_tracer.cs.glsl
#include "../cbuffer.hlsl.h"
#include "../shader_resource_defines.hlsl.h"

layout(rgba32f, binding = 0) uniform image2D u_PathTracerOutputImage;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../shared_path_tracer.h"

void main() {
    ivec2 iPixelCoords = ivec2(gl_GlobalInvocationID.xy);

    vec2 fPixelCoords = vec2(float(iPixelCoords.x), float(iPixelCoords.y));
    ivec2 dims = imageSize(u_PathTracerOutputImage);

    uint seed = uint(uint(iPixelCoords.x) * uint(1973) + uint(iPixelCoords.y) * uint(9277) + uint(c_frameIndex) * uint(26699)) | uint(1);

    // [-0.5, 0.5]
    vec2 jitter = vec2(Random(seed), Random(seed)) - 0.5f;

    vec3 rayDir;
    {
        // screen position from [-1, 1]
        vec2 uvJitter = (fPixelCoords + jitter) / dims;
        vec2 screen = 2.0 * uvJitter - 1.0;

        // adjust for aspect ratio
        vec2 resolution = vec2(float(dims.x), float(dims.y));
        float aspectRatio = resolution.x / resolution.y;
        screen.y /= aspectRatio;
        float halfFov = c_cameraFovDegree;
        float camDistance = tan(halfFov * MY_PI / 180.0);
        rayDir = vec3(screen, camDistance);
        rayDir = normalize(mat3(c_cameraRight, c_cameraUp, c_cameraForward) * rayDir);
    }

    Ray ray;
    ray.origin = c_cameraPosition;
    ray.direction = rayDir;
    ray.invDir = 1.0f / ray.direction;
    ray.t = RAY_T_MAX;

    vec3 radiance = RayColor(ray, seed);

    vec4 final_color = vec4(radiance, 1.0);

#if FORCE_UPDATE == 0
    if (c_sceneDirty == 0) {
        vec4 accumulated = imageLoad(u_PathTracerOutputImage, iPixelCoords);
        float weight = accumulated.a;
        vec3 new_color = radiance + weight * accumulated.rgb;
        float new_weight = accumulated.a + 1.0;
        new_color /= new_weight;
        final_color = vec4(new_color, new_weight);
    }
#endif

    imageStore(u_PathTracerOutputImage, iPixelCoords, final_color);
}

#if 0

vec3 RayColor(inout Ray ray, inout uint state) {
    vec3 radiance = vec3(0.0);
    vec3 throughput = vec3(1.0);

    for (int i = 0; i < MAX_BOUNCE; ++i) {
        bool anyHit = HitScene(ray);
        if (anyHit) {
            ray.origin = ray.origin + ray.t * ray.direction;
            ray.t = RAY_T_MAX;
            gpu_material_t mat = GlobalMaterials[ray.material_id];
            vec3 diffuseColor = vec3(1.0);
            if (mat.has_base_color_map == 1) {
                vec4 sampled_color = texture(mat.base_color_map_handle, ray.hit_uv);
                if (sampled_color.a <= 0.01) {
                    continue;
                }
                diffuseColor = sampled_color.rgb;
            } else {
                diffuseColor *= mat.albedo;
            }

            float metallic = mat.reflect_chance;
            float roughness = mat.roughness;
            if (mat.has_material_map == 1) {
                vec4 sampled_color = texture(mat.material_map_handle, ray.hit_uv);
                metallic = sampled_color.b;
                roughness = sampled_color.g;
            }

            float specularChance = Random(state) > metallic ? 0.0 : 1.0;

            vec3 diffuseDir = normalize(ray.hit_normal + RandomUnitVector(state));
            vec3 reflectDir = reflect(ray.direction, ray.hit_normal);
            reflectDir = normalize(mix(reflectDir, diffuseDir, roughness * roughness));
            ray.direction = normalize(mix(diffuseDir, reflectDir, specularChance));

            radiance += mat.emissive * throughput;
            throughput *= diffuseColor;

        } else {
            vec2 uv = SampleSphericalMap(normalize(ray.direction));
            radiance += texture(c_hdrEnvMap, uv).rgb * throughput;
            // radiance += vec3(0.5, 0.7, 1.0) * throughput;
            break;
        }
    }

    return radiance;
}

uint g_seed;
#endif
