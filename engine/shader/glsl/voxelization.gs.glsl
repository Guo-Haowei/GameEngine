/// File: voxelization.gs.glsl
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 pass_positions[];
in vec3 pass_normals[];
in vec2 pass_uvs[];

#include "../cbuffer.hlsl.h"

out vec3 pass_position;
out vec3 pass_normal;
out vec2 pass_uv;

void main() {
    vec3 triangle_normal = abs(pass_normals[0] + pass_normals[1] + pass_normals[2]);

    // @TODO: there's a bug stretching voxel along x axis
    uint maxi = triangle_normal.x > triangle_normal.y ? 0 : 1;
    maxi = triangle_normal.z > maxi ? 2 : maxi;

    vec3 output_positions[3];

    for (uint i = 0; i < 3; ++i) {
        output_positions[i] = (pass_positions[i] - c_voxelWorldCenter) / c_voxelWorldSizeHalf;
        if (maxi == 0) {
            output_positions[i].xyz = output_positions[i].zyx;
        } else if (maxi == 1) {
            output_positions[i].xyz = output_positions[i].xzy;
        }
    }

    // stretch the triangle to fill more one more texel
    vec2 side0N = normalize(output_positions[1].xy - output_positions[0].xy);
    vec2 side1N = normalize(output_positions[2].xy - output_positions[1].xy);
    vec2 side2N = normalize(output_positions[0].xy - output_positions[2].xy);

    output_positions[0].xy += normalize(side2N - side0N) * c_texelSize;
    output_positions[1].xy += normalize(side0N - side1N) * c_texelSize;
    output_positions[2].xy += normalize(side1N - side2N) * c_texelSize;

    for (uint i = 0; i < 3; ++i) {
        pass_position = pass_positions[i];
        pass_normal = pass_normals[i];
        pass_uv = pass_uvs[i];
        gl_Position = vec4(output_positions[i].xy, 1.0, 1.0);
        EmitVertex();
    }
    EndPrimitive();
}
