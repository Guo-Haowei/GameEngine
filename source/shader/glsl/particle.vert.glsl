#include "../cbuffer.h"
#include "../vsinput.glsl.h"

layout(std430, binding = 0) buffer ParticleData {
    vec4 globalParticles[];
};

void main() {
    vec4 transform = globalParticles[gl_InstanceID];
    float tx = transform.x;
    float ty = transform.y;
    float tz = transform.z;
    float s = transform.w;

    mat4 scale_matrix = mat4(
        s, 0.0, 0.0, 0.0,
        0.0, s, 0.0, 0.0,
        0.0, 0.0, s, 0.0,
        0.0, 0.0, 0.0, 1.0);

    mat4 model_matrix = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        tx, ty, tz, 1.0);

    // make sure always face to screen
    // https://blog.42yeah.is/opengl/rendering/2023/06/24/opengl-billboards.html
    mat4 view_model_matrix = g_view_matrix * model_matrix;
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
    gl_Position = g_projection_matrix * position;
}

