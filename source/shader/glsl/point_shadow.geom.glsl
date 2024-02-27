#include "../cbuffer.h"

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

out vec4 pass_position;

void main() {
    for (int face = 0; face < 6; ++face) {
        // built-in variable that specifies to which face we render.
        gl_Layer = face;
        for (int i = 0; i < 3; ++i) {
            pass_position = gl_in[i].gl_Position;
            gl_Position = c_lights[c_light_index].matrices[face] * pass_position;
            EmitVertex();
        }
        EndPrimitive();
    }
}