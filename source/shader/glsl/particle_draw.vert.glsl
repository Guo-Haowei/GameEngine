/// File: particle_draw.vert.glsl
#include "../vsinput.glsl.h"
#include "../cbuffer.hlsl.h"
#include "../particle_defines.hlsl.h"

layout(location = 0) out vec3 pass_color;

void main() {
    Particle particle = GlobalParticleData[gl_InstanceID];

    if (particle.lifeRemaining <= 0.0) {
        gl_Position = vec4(1000.0, 1000.0, 1000.0, 1.0);
        return;
    }

    vec3 t = particle.position.xyz;
    float s = particle.scale;

    mat4 scale_matrix = mat4(
        s, 0.0, 0.0, 0.0,
        0.0, s, 0.0, 0.0,
        0.0, 0.0, s, 0.0,
        0.0, 0.0, 0.0, 1.0);

    mat4 model_matrix = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        t.x, t.y, t.z, 1.0);

    // make sure always face to screen
    // https://blog.42yeah.is/opengl/rendering/2023/06/24/opengl-billboards.html
    mat4 view_model_matrix = c_viewMatrix * model_matrix;
    view_model_matrix[0][0] = 1.0;
    view_model_matrix[0][1] = 0.0;
    view_model_matrix[0][2] = 0.0;
    view_model_matrix[1][0] = 0.0;
    view_model_matrix[1][1] = 1.0;
    view_model_matrix[1][2] = 0.0;
    view_model_matrix[2][0] = 0.0;
    view_model_matrix[2][1] = 0.0;
    view_model_matrix[2][2] = 1.0;

    vec4 position = scale_matrix * vec4(in_position, 1.0);
    position = view_model_matrix * position;

    pass_color = particle.color.rgb;
    gl_Position = c_projectionMatrix * position;
}

