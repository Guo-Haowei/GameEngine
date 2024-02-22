#include "geometry.h"

namespace my {

MeshComponent make_cube_mesh(const vec3& p_scale) {
    MeshComponent mesh;
    // clang-format off
    constexpr uint32_t indices[] = {
        0,          1,          2,          0,          2,          3,
        0 + 4,      2 + 4,      1 + 4,      0 + 4,      3 + 4,      2 + 4,  // swapped winding
        0 + 4 * 2,  1 + 4 * 2,  2 + 4 * 2,  0 + 4 * 2,  2 + 4 * 2,  3 + 4 * 2,
        0 + 4 * 3,  2 + 4 * 3,  1 + 4 * 3,  0 + 4 * 3,  3 + 4 * 3,  2 + 4 * 3, // swapped winding
        0 + 4 * 4,  2 + 4 * 4,  1 + 4 * 4,  0 + 4 * 4,  3 + 4 * 4,  2 + 4 * 4, // swapped winding
        0 + 4 * 5,  1 + 4 * 5,  2 + 4 * 5,  0 + 4 * 5,  2 + 4 * 5,  3 + 4 * 5,
    };
    // clang-format on

    const vec3& s = p_scale;
    mesh.positions = {
        // -Z
        vec3(-s.x, +s.y, -s.z),
        vec3(-s.x, -s.y, -s.z),
        vec3(+s.x, -s.y, -s.z),
        vec3(+s.x, +s.y, -s.z),

        // +Z
        vec3(-s.x, +s.y, +s.z),
        vec3(-s.x, -s.y, +s.z),
        vec3(+s.x, -s.y, +s.z),
        vec3(+s.x, +s.y, +s.z),

        // -X
        vec3(-s.x, -s.y, +s.z),
        vec3(-s.x, -s.y, -s.z),
        vec3(-s.x, +s.y, -s.z),
        vec3(-s.x, +s.y, +s.z),

        // +X
        vec3(+s.x, -s.y, +s.z),
        vec3(+s.x, -s.y, -s.z),
        vec3(+s.x, +s.y, -s.z),
        vec3(+s.x, +s.y, +s.z),

        // -Y
        vec3(-s.x, -s.y, +s.z),
        vec3(-s.x, -s.y, -s.z),
        vec3(+s.x, -s.y, -s.z),
        vec3(+s.x, -s.y, +s.z),

        // +Y
        vec3(-s.x, +s.y, +s.z),
        vec3(-s.x, +s.y, -s.z),
        vec3(+s.x, +s.y, -s.z),
        vec3(+s.x, +s.y, +s.z),
    };

    mesh.texcoords_0 = {
        vec2(0, 0),
        vec2(0, 1),
        vec2(1, 1),
        vec2(1, 0),

        vec2(0, 0),
        vec2(0, 1),
        vec2(1, 1),
        vec2(1, 0),

        vec2(0, 0),
        vec2(0, 1),
        vec2(1, 1),
        vec2(1, 0),

        vec2(0, 0),
        vec2(0, 1),
        vec2(1, 1),
        vec2(1, 0),

        vec2(0, 0),
        vec2(0, 1),
        vec2(1, 1),
        vec2(1, 0),

        vec2(0, 0),
        vec2(0, 1),
        vec2(1, 1),
        vec2(1, 0),
    };

    mesh.normals = {
        vec3(0, 0, -1),
        vec3(0, 0, -1),
        vec3(0, 0, -1),
        vec3(0, 0, -1),

        vec3(0, 0, 1),
        vec3(0, 0, 1),
        vec3(0, 0, 1),
        vec3(0, 0, 1),

        vec3(-1, 0, 0),
        vec3(-1, 0, 0),
        vec3(-1, 0, 0),
        vec3(-1, 0, 0),

        vec3(1, 0, 0),
        vec3(1, 0, 0),
        vec3(1, 0, 0),
        vec3(1, 0, 0),

        vec3(0, -1, 0),
        vec3(0, -1, 0),
        vec3(0, -1, 0),
        vec3(0, -1, 0),

        vec3(0, 1, 0),
        vec3(0, 1, 0),
        vec3(0, 1, 0),
        vec3(0, 1, 0),
    };

    for (int i = 0; i < array_length(indices); i += 3) {
        mesh.indices.push_back(indices[i]);
        mesh.indices.push_back(indices[i + 2]);
        mesh.indices.push_back(indices[i + 1]);
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = array_length(indices);
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.create_render_data();
    return mesh;
}

MeshComponent make_sphere_mesh(float radius, int rings, int sectors) {
    unused(radius);
    unused(rings);
    unused(sectors);

    MeshComponent mesh;

    const int p_sectors = 80;  // vertex grid size
    const int p_rings = 40;

    mesh.indices.resize(p_sectors * (p_rings - 1) * 6);

    const float M_PI = glm::pi<float>();

    float a, b, da, db;
    float p_radius = 0.5f;
    int ia, ib, ix, iy;
    da = 2.0f * M_PI / float(p_sectors);
    db = M_PI / float(p_rings - 1);
    // [Generate sphere point data]
    // spherical angles a,b covering whole sphere surface
    for (ix = 0, b = -0.5 * M_PI, ib = 0; ib < p_rings; ++ib, b += db)
        for (a = 0.0, ia = 0; ia < p_sectors; ia++, a += da, ix += 3) {
            // unit sphere
            const float x = glm::cos(b) * glm::cos(a);
            const float y = glm::cos(b) * glm::sin(a);
            const float z = glm::sin(b);

            const vec3 normal = glm::normalize(vec3{ x, y, z });
            mesh.positions.push_back(p_radius * normal);
            mesh.normals.push_back(normal);
            mesh.texcoords_0.push_back(vec2(0));
        }
    // [Generate GL_TRIANGLE indices]
    for (ix = 0, iy = 0, ib = 1; ib < p_rings; ib++) {
        for (ia = 1; ia < p_sectors; ia++, iy++) {
            // first half of QUAD
            mesh.indices[ix] = iy;
            ix++;
            mesh.indices[ix] = iy + 1;
            ix++;
            mesh.indices[ix] = iy + p_sectors;
            ix++;
            // second half of QUAD
            mesh.indices[ix] = iy + p_sectors;
            ix++;
            mesh.indices[ix] = iy + 1;
            ix++;
            mesh.indices[ix] = iy + p_sectors + 1;
            ix++;
        }
        // first half of QUAD
        mesh.indices[ix] = iy;
        ix++;
        mesh.indices[ix] = iy + 1 - p_sectors;
        ix++;
        mesh.indices[ix] = iy + p_sectors;
        ix++;
        // second half of QUAD
        mesh.indices[ix] = iy + p_sectors;
        ix++;
        mesh.indices[ix] = iy - p_sectors + 1;
        ix++;
        mesh.indices[ix] = iy + 1;
        ix++;
        iy++;
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = (uint32_t)mesh.indices.size();
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.create_render_data();
    return mesh;
}

/**
 *        E__________________ H
 *       /|                 /|
 *      / |                / |
 *     /  |               /  |
 *   A/___|______________/D  |
 *    |   |              |   |
 *    |   |              |   |
 *    |   |              |   |
 *    |  F|______________|___|G
 *    |  /               |  /
 *    | /                | /
 *   B|/_________________|C
 *
 */

// clang-format off
enum { A = 0, B = 1, C = 2, D = 3, E = 4, F = 5, G = 6, H = 7 };
// clang-format on

MeshComponent make_plane_mesh(float size) {
    MeshComponent mesh;
    mesh.positions = {
        vec3(-size, +size, 0.0f),  // A
        vec3(-size, -size, 0.0f),  // B
        vec3(+size, -size, 0.0f),  // C
        vec3(+size, +size, 0.0f),  // D
    };

    mesh.texcoords_0 = {
        vec2(1, 0),  // top-left
        vec2(0, 0),  // bottom-left
        vec2(0, 1),  // bottom-right
        vec2(1, 1),  // top-right
    };

    mesh.indices = {
        A, B, D,  // ABD
        D, B, C,  // DBC
    };

    return mesh;
}

MeshComponent make_box_mesh(float size) {
    MeshComponent mesh;
    mesh.positions = {
        vec3(-size, +size, +size),  // A
        vec3(-size, -size, +size),  // B
        vec3(+size, -size, +size),  // C
        vec3(+size, +size, +size),  // D
        vec3(-size, +size, -size),  // E
        vec3(-size, -size, -size),  // F
        vec3(+size, -size, -size),  // G
        vec3(+size, +size, -size),  // H
    };

    mesh.indices = {
        A, B, D,  // ABD
        D, B, C,  // DBC
        E, H, F,  // EHF
        H, G, F,  // HGF
        D, C, G,  // DCG
        D, G, H,  // DGH
        A, F, B,  // AFB
        A, E, F,  // AEF
        A, D, H,  // ADH
        A, H, E,  // AHE
        B, F, G,  // BFG
        B, G, C,  // BGC
    };

    return mesh;
}

// load scene
MeshComponent make_box_wireframe_mesh(float size) {
    MeshComponent mesh;
    mesh.positions = {
        vec3(-size, +size, +size),  // A
        vec3(-size, -size, +size),  // B
        vec3(+size, -size, +size),  // C
        vec3(+size, +size, +size),  // D
        vec3(-size, +size, -size),  // E
        vec3(-size, -size, -size),  // F
        vec3(+size, -size, -size),  // G
        vec3(+size, +size, -size),  // H
    };

    mesh.indices = { A, B, B, C, C, D, D, A, E, F, F, G, G, H, H, E, A, E, B, F, D, H, C, G };

    return mesh;
}

}  // namespace my
