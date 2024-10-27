#include "../cbuffer.h"
#include "../vsinput.glsl.h"

void main() {
    vec4 transform = globalPatricleTransforms[gl_InstanceID];
    float tx = transform.x;
    float ty = transform.y;
    float tz = transform.z;
    float s = transform.w;

    mat4 world_matrix = mat4(
        s, 0.0, 0.0, 0.0,
        0.0, s, 0.0, 0.0,
        0.0, 0.0, s, 0.0,
        tx, ty, tz, 1.0);

    vec4 position = world_matrix * vec4(in_position, 1.0);

    gl_Position = g_projection_matrix * (g_view_matrix * position);
}
