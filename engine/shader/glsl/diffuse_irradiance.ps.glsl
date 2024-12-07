/// File: diffuse_irradiance.ps.glsl
#include "../cbuffer.hlsl.h"
#include "../shader_resource_defines.hlsl.h"

layout(location = 0) out vec4 out_color;
in vec3 out_var_POSITION;

const float sampleStep = 0.025;

void main() {
    vec3 N = normalize(out_var_POSITION);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, N);
    up = cross(N, right);

    vec3 irradiance = vec3(0.0);
    float samples = 0.0;

    for (float phi = 0.0; phi < 2.0 * MY_PI; phi += sampleStep) {
        for (float theta = 0.0; theta < 0.5 * MY_PI; theta += sampleStep) {
            float xdir = sin(theta) * cos(phi);
            float ydir = sin(theta) * sin(phi);
            float zdir = cos(theta);
            vec3 sampleVec = xdir * right + ydir * up + zdir * N;
            irradiance += textureLod(t_Skybox, sampleVec, 0.0).rgb * cos(theta) * sin(theta);
            samples += 1.0;
        }
    }

    irradiance = MY_PI * irradiance * (1.0 / samples);
    out_color = vec4(irradiance, 1.0);
}