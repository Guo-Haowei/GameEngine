#include "geometry.h"

namespace my {

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

MeshComponent make_plane_mesh(const vec3& p_scale) {
    MeshComponent mesh;
    const float x = p_scale.x;
    const float y = p_scale.y;
    mesh.positions = {
        vec3(-x, +y, 0.0f),  // A
        vec3(-x, -y, 0.0f),  // B
        vec3(+x, -y, 0.0f),  // C
        vec3(+x, +y, 0.0f),  // D
    };

    const vec3 normal{ 0, 0, 1 };
    mesh.normals = {
        normal,
        normal,
        normal,
        normal,
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

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(mesh.indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.create_render_data();
    return mesh;
}

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

MeshComponent make_sphere_mesh(float p_radius, int p_rings, int p_sectors) {
    MeshComponent mesh;

    auto& indices = mesh.indices;
    constexpr float pi = glm::pi<float>();
    for (int step_x = 0; step_x <= p_sectors; ++step_x) {
        for (int step_y = 0; step_y <= p_rings; ++step_y) {
            const float x_seg = (float)step_x / (float)p_sectors;
            const float y_seg = (float)step_y / (float)p_rings;
            const vec3 normal{
                std::cos(x_seg * 2.0f * pi) * std::sin(y_seg * pi),
                std::cos(y_seg * pi),
                std::sin(x_seg * 2.0f * pi) * std::sin(y_seg * pi)
            };

            mesh.positions.push_back(p_radius * normal);
            mesh.normals.push_back(normal);
            mesh.texcoords_0.push_back(vec2(x_seg, y_seg));
        }
    }

    for (int y = 0; y < p_rings; ++y) {
        for (int x = 0; x < p_sectors; ++x) {
            /*
            a - b
            |   |
            c - d
            */
            uint32_t a = (y * (p_sectors + 1) + x);
            uint32_t b = (y * (p_sectors + 1) + x + 1);
            uint32_t c = ((y + 1) * (p_sectors + 1) + x);
            uint32_t d = ((y + 1) * (p_sectors + 1) + x + 1);

            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(b);

            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(d);
        }
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.create_render_data();
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

MeshComponent make_sky_box_mesh() {
    float size = 1.0f;
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
        A, D, B,  // ABD
        D, C, B,  // DBC
        E, F, H,  // EHF
        H, F, G,  // HGF
        D, G, C,  // DCG
        D, H, G,  // DGH
        A, B, F,  // AFB
        A, F, E,  // AEF
        A, H, D,  // ADH
        A, E, H,  // AHE
        B, G, F,  // BFG
        B, C, G,  // BGC
        // A, B, D,  // ABD
        // D, B, C,  // DBC
        // E, H, F,  // EHF
        // H, G, F,  // HGF
        // D, C, G,  // DCG
        // D, G, H,  // DGH
        // A, F, B,  // AFB
        // A, E, F,  // AEF
        // A, D, H,  // ADH
        // A, H, E,  // AHE
        // B, F, G,  // BFG
        // B, G, C,  // BGC
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
