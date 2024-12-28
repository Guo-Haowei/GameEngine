#include "geometry.h"

#include "matrix_transform.h"
// @TODO: refactor
#include "detail/matrix.h"

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

MeshComponent MakePlaneMesh(const Vector3f& p_scale) {
    const float x = p_scale.x;
    const float y = p_scale.y;
    Vector3f a(-x, +y, 0.0f);  // A
    Vector3f b(-x, -y, 0.0f);  // B
    Vector3f c(+x, -y, 0.0f);  // C
    Vector3f d(+x, +y, 0.0f);  // D
    return MakePlaneMesh(a, b, c, d);
}

MeshComponent MakePlaneMesh(const Vector3f& p_point_0,
                            const Vector3f& p_point_1,
                            const Vector3f& p_point_2,
                            const Vector3f& p_point_3) {
    MeshComponent mesh;
    mesh.positions = {
        p_point_0,
        p_point_1,
        p_point_2,
        p_point_3,
    };

    const Vector3f normal = normalize(cross(p_point_0 - p_point_1, p_point_0 - p_point_2));
    mesh.normals = {
        normal,
        normal,
        normal,
        normal,
    };

    mesh.texcoords_0 = {
        Vector2f(0, 1),  // top-left
        Vector2f(0, 0),  // bottom-left
        Vector2f(1, 0),  // bottom-right
        Vector2f(1, 1),  // top-right
    };

    // clang-format off
    mesh.indices = {
#if 1
        A, B, D,  // ABD
        D, B, C,  // DBC
#else
        A, D, B, // ADB
        D, C, B, // DBC
#endif
    };
    // clang-format on

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(mesh.indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeCubeMesh(const Vector3f& p_scale) {
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

    const Vector3f& s = p_scale;
    mesh.positions = {
        // -Z
        Vector3f(-s.x, +s.y, -s.z),
        Vector3f(-s.x, -s.y, -s.z),
        Vector3f(+s.x, -s.y, -s.z),
        Vector3f(+s.x, +s.y, -s.z),

        // +Z
        Vector3f(-s.x, +s.y, +s.z),
        Vector3f(-s.x, -s.y, +s.z),
        Vector3f(+s.x, -s.y, +s.z),
        Vector3f(+s.x, +s.y, +s.z),

        // -X
        Vector3f(-s.x, -s.y, +s.z),
        Vector3f(-s.x, -s.y, -s.z),
        Vector3f(-s.x, +s.y, -s.z),
        Vector3f(-s.x, +s.y, +s.z),

        // +X
        Vector3f(+s.x, -s.y, +s.z),
        Vector3f(+s.x, -s.y, -s.z),
        Vector3f(+s.x, +s.y, -s.z),
        Vector3f(+s.x, +s.y, +s.z),

        // -Y
        Vector3f(-s.x, -s.y, +s.z),
        Vector3f(-s.x, -s.y, -s.z),
        Vector3f(+s.x, -s.y, -s.z),
        Vector3f(+s.x, -s.y, +s.z),

        // +Y
        Vector3f(-s.x, +s.y, +s.z),
        Vector3f(-s.x, +s.y, -s.z),
        Vector3f(+s.x, +s.y, -s.z),
        Vector3f(+s.x, +s.y, +s.z),
    };

    mesh.texcoords_0 = {
        Vector2f(0, 0),
        Vector2f(0, 1),
        Vector2f(1, 1),
        Vector2f(1, 0),

        Vector2f(0, 0),
        Vector2f(0, 1),
        Vector2f(1, 1),
        Vector2f(1, 0),

        Vector2f(0, 0),
        Vector2f(0, 1),
        Vector2f(1, 1),
        Vector2f(1, 0),

        Vector2f(0, 0),
        Vector2f(0, 1),
        Vector2f(1, 1),
        Vector2f(1, 0),

        Vector2f(0, 0),
        Vector2f(0, 1),
        Vector2f(1, 1),
        Vector2f(1, 0),

        Vector2f(0, 0),
        Vector2f(0, 1),
        Vector2f(1, 1),
        Vector2f(1, 0),
    };

    mesh.normals = {
        Vector3f(0, 0, -1),
        Vector3f(0, 0, -1),
        Vector3f(0, 0, -1),
        Vector3f(0, 0, -1),

        Vector3f(0, 0, 1),
        Vector3f(0, 0, 1),
        Vector3f(0, 0, 1),
        Vector3f(0, 0, 1),

        Vector3f(-1, 0, 0),
        Vector3f(-1, 0, 0),
        Vector3f(-1, 0, 0),
        Vector3f(-1, 0, 0),

        Vector3f(1, 0, 0),
        Vector3f(1, 0, 0),
        Vector3f(1, 0, 0),
        Vector3f(1, 0, 0),

        Vector3f(0, -1, 0),
        Vector3f(0, -1, 0),
        Vector3f(0, -1, 0),
        Vector3f(0, -1, 0),

        Vector3f(0, 1, 0),
        Vector3f(0, 1, 0),
        Vector3f(0, 1, 0),
        Vector3f(0, 1, 0),
    };

    for (int i = 0; i < array_length(indices); i += 3) {
        mesh.indices.emplace_back(indices[i]);
        mesh.indices.emplace_back(indices[i + 2]);
        mesh.indices.emplace_back(indices[i + 1]);
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = array_length(indices);
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeCubeMesh(const std::array<Vector3f, 8>& p_points) {
    MeshComponent mesh;

    const Vector3f& a = p_points[A];
    const Vector3f& b = p_points[B];
    const Vector3f& c = p_points[C];
    const Vector3f& d = p_points[D];
    const Vector3f& e = p_points[E];
    const Vector3f& f = p_points[F];
    const Vector3f& g = p_points[G];
    const Vector3f& h = p_points[H];
    // clang-format off
    mesh.positions = {
        // front
        a, b, c, d,
        // back
        e, h, g ,f,
        // left
        a, e, f, b,
        // right
        c, g, h, d,
        // up
        a, d, h, e,
        // down
        b, f, g, c,
    };
    // clang-format on

    mesh.indices.clear();
    mesh.normals.clear();
    for (int i = 0; i < 24; i += 4) {
        const Vector3f& A = mesh.positions[i];
        const Vector3f& B = mesh.positions[i + 1];
        const Vector3f& C = mesh.positions[i + 2];
        // const Vector3f& D = mesh.positions[i + 3];
        Vector3f AB = B - A;
        Vector3f AC = C - A;
        Vector3f normal = normalize(cross(AB, AC));

        mesh.normals.emplace_back(normal);
        mesh.normals.emplace_back(normal);
        mesh.normals.emplace_back(normal);
        mesh.normals.emplace_back(normal);

        mesh.indices.emplace_back(i);
        mesh.indices.emplace_back(i + 1);
        mesh.indices.emplace_back(i + 2);

        mesh.indices.emplace_back(i);
        mesh.indices.emplace_back(i + 2);
        mesh.indices.emplace_back(i + 3);

        // TODO: fix dummy uv
        mesh.texcoords_0.push_back(Vector2f::Zero);
        mesh.texcoords_0.push_back(Vector2f::Zero);
        mesh.texcoords_0.push_back(Vector2f::Zero);
        mesh.texcoords_0.push_back(Vector2f::Zero);
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(mesh.indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeTetrahedronMesh(float p_size) {
    // Vertex data for a tetrahedron (4 vertices, each with x, y, z coordinates)
    constexpr float h = 2.0f / 2.449f;
    Vector3f vertices[] = {
        p_size * Vector3f(+0, +h, +1),  // top front
        p_size * Vector3f(+0, +h, -1),  // top back
        p_size * Vector3f(-1, -h, +0),  // bottom left vertex
        p_size * Vector3f(+1, -h, +0),  // bottom right vertex
    };

    static const uint32_t indices[] = {
        0, 2, 3,  // face 1
        0, 3, 1,  // face 2
        0, 1, 2,  // face 3
        1, 3, 2,  // face 4
    };

    MeshComponent mesh;

    for (int i = 0; i < array_length(indices); i += 3) {
        Vector3f A = vertices[indices[i]];
        Vector3f B = vertices[indices[i + 1]];
        Vector3f C = vertices[indices[i + 2]];

        Vector3f normal = normalize(cross(A - B, A - C));

        mesh.positions.emplace_back(A);
        mesh.positions.emplace_back(B);
        mesh.positions.emplace_back(C);

        mesh.normals.emplace_back(normal);
        mesh.normals.emplace_back(normal);
        mesh.normals.emplace_back(normal);

        mesh.indices.emplace_back(i);
        mesh.indices.emplace_back(i + 1);
        mesh.indices.emplace_back(i + 2);

        mesh.texcoords_0.emplace_back(Vector2f());
        mesh.texcoords_0.emplace_back(Vector2f());
        mesh.texcoords_0.emplace_back(Vector2f());
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(mesh.indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeSphereMesh(float p_radius, int p_rings, int p_sectors) {
    MeshComponent mesh;

    auto& indices = mesh.indices;
    constexpr float pi = glm::pi<float>();
    for (int step_x = 0; step_x <= p_sectors; ++step_x) {
        for (int step_y = 0; step_y <= p_rings; ++step_y) {
            const float x_seg = (float)step_x / (float)p_sectors;
            const float y_seg = (float)step_y / (float)p_rings;
            const Vector3f normal{
                std::cos(x_seg * 2.0f * pi) * std::sin(y_seg * pi),
                std::cos(y_seg * pi),
                std::sin(x_seg * 2.0f * pi) * std::sin(y_seg * pi)
            };

            mesh.positions.emplace_back(p_radius * normal);
            mesh.normals.emplace_back(normal);
            mesh.texcoords_0.emplace_back(Vector2f(x_seg, y_seg));
        }
    }

    for (int y = 0; y < p_rings; ++y) {
        for (int x = 0; x < p_sectors; ++x) {
            /* a - b
               |   |
               c - d */
            uint32_t a = (y * (p_sectors + 1) + x);
            uint32_t b = (y * (p_sectors + 1) + x + 1);
            uint32_t c = ((y + 1) * (p_sectors + 1) + x);
            uint32_t d = ((y + 1) * (p_sectors + 1) + x + 1);

            indices.emplace_back(a);
            indices.emplace_back(c);
            indices.emplace_back(b);

            indices.emplace_back(b);
            indices.emplace_back(c);
            indices.emplace_back(d);
        }
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeCylinderMesh(float p_radius,
                               float p_height,
                               int p_sectors,
                               int p_height_sector) {
    MeshComponent mesh;

    auto& indices = mesh.indices;
    constexpr float pi = glm::pi<float>();

    std::array<float, 2> heights = { 0.5f * p_height, -0.5f * p_height };

    // cylinder side
    float height_step = (float)p_height / p_height_sector;
    for (float y = heights[1]; y < heights[0]; y += height_step) {
        uint32_t point_offset = (uint32_t)mesh.positions.size();
        for (int index = 0; index < p_sectors; ++index) {
            float angle_1 = 2.0f * pi * index / p_sectors;
            float x_1 = p_radius * std::cos(angle_1);
            float z_1 = p_radius * std::sin(angle_1);
            float angle_2 = 2.0f * pi * (index + 1) / p_sectors;
            float x_2 = p_radius * std::cos(angle_2);
            float z_2 = p_radius * std::sin(angle_2);

            Vector3f point_1(x_1, y, z_1);
            Vector3f point_2(x_1, y + height_step, z_1);

            Vector3f point_3(x_2, y, z_2);
            Vector3f point_4(x_2, y + height_step, z_2);

            Vector3f AB = point_1 - point_2;
            Vector3f AC = point_1 - point_3;
            Vector3f normal = normalize(cross(AB, AC));

            mesh.positions.emplace_back(point_1);
            mesh.positions.emplace_back(point_2);
            mesh.positions.emplace_back(point_3);
            mesh.positions.emplace_back(point_4);

            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);
            mesh.normals.emplace_back(normal);

            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());
            mesh.texcoords_0.emplace_back(Vector2f());

            const uint32_t a = point_offset + 4 * index;
            const uint32_t c = point_offset + 4 * index + 1;
            const uint32_t b = point_offset + 4 * index + 2;
            [[maybe_unused]] const uint32_t d = point_offset + 4 * index + 3;

            indices.emplace_back(a);
            indices.emplace_back(c);
            indices.emplace_back(b);

            indices.emplace_back(c);
            indices.emplace_back(d);
            indices.emplace_back(b);
        }
    }

    // cylinder circles
    for (float height : heights) {
        uint32_t offset = static_cast<uint32_t>(mesh.positions.size());

        Vector3f normal = normalize(Vector3f(0.0f, height, 0.0f));

        for (int index = 0; index <= p_sectors; ++index) {
            float angle = 2.0f * pi * index / p_sectors;
            float x = p_radius * glm::cos(angle);
            float z = p_radius * glm::sin(angle);

            Vector3f point(x, height, z);

            mesh.positions.emplace_back(point);
            mesh.normals.emplace_back(normal);
            mesh.texcoords_0.emplace_back(Vector2f());
        }

        mesh.positions.emplace_back(Vector3f(0.0f, height, 0.0f));
        mesh.normals.emplace_back(normal);
        mesh.texcoords_0.emplace_back(Vector2f());

        uint32_t center_index = static_cast<uint32_t>(mesh.positions.size()) - 1;
        for (int index = 0; index < p_sectors; ++index) {
            if (height < 0) {
                indices.emplace_back(offset + index);
                indices.emplace_back(offset + index + 1);
                indices.emplace_back(center_index);
            } else {
                indices.emplace_back(offset + index + 1);
                indices.emplace_back(offset + index);
                indices.emplace_back(center_index);
            }
        }
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeConeMesh(float p_radius,
                           float p_height,
                           int p_sectors) {
    MeshComponent mesh;

    auto& indices = mesh.indices;
    constexpr float pi = glm::pi<float>();

    const float height_half = 0.5f * p_height;
    const Vector3f apex{ 0.0f, height_half, 0.0f };

    // cone side
    for (int index = 0; index < p_sectors; ++index) {
        const float angle_1 = 2.0f * pi * index / p_sectors;
        const float x_1 = p_radius * glm::cos(angle_1);
        const float z_1 = p_radius * glm::sin(angle_1);

        const float angle_2 = 2.0f * pi * (index + 1) / p_sectors;
        const float x_2 = p_radius * glm::cos(angle_2);
        const float z_2 = p_radius * glm::sin(angle_2);

        Vector3f point_1(x_1, -height_half, z_1);
        Vector3f point_2(x_2, -height_half, z_2);

        // Vector3f normal = glm::normalize(Vector3f(x, 0.0f, z));
        Vector3f AB = point_1 - apex;
        Vector3f AC = point_2 - apex;
        Vector3f normal = normalize(cross(AC, AB));

        mesh.positions.emplace_back(point_1);
        mesh.positions.emplace_back(apex);
        mesh.positions.emplace_back(point_2);

        mesh.normals.emplace_back(normal);
        mesh.normals.emplace_back(normal);
        mesh.normals.emplace_back(normal);

        mesh.indices.emplace_back(3 * index);
        mesh.indices.emplace_back(3 * index + 1);
        mesh.indices.emplace_back(3 * index + 2);

        // @TODO: fix dummy uv
        mesh.texcoords_0.emplace_back(Vector2f());
        mesh.texcoords_0.emplace_back(Vector2f());
        mesh.texcoords_0.emplace_back(Vector2f());
    }

#if 0
    for (int index = 0; index < p_sectors; ++index) {
        /* a - b
           |   |
           c - d */
        const uint32_t a = 2 * index;
        const uint32_t c = 2 * index + 1;
        const uint32_t b = 2 * index + 2;
        const uint32_t d = 2 * index + 3;

        indices.emplace_back(a);
        indices.emplace_back(b);
        indices.emplace_back(c);

        indices.emplace_back(c);
        indices.emplace_back(b);
        indices.emplace_back(d);
    }
#endif

    // cylinder circles
    {
        uint32_t offset = static_cast<uint32_t>(mesh.positions.size());

        Vector3f normal(0, -1, 0);

        for (int index = 0; index <= p_sectors; ++index) {
            float angle = 2.0f * pi * index / p_sectors;
            float x = p_radius * glm::cos(angle);
            float z = p_radius * glm::sin(angle);

            Vector3f point(x, -height_half, z);

            mesh.positions.emplace_back(point);
            mesh.normals.emplace_back(normal);
            mesh.texcoords_0.emplace_back(Vector2f());
        }

        // center
        mesh.positions.emplace_back(Vector3f(0.0f, -height_half, 0.0f));
        mesh.normals.emplace_back(normal);
        mesh.texcoords_0.emplace_back(Vector2f());

        uint32_t center_index = static_cast<uint32_t>(mesh.positions.size()) - 1;
        for (int index = 0; index < p_sectors; ++index) {
            indices.emplace_back(offset + index);
            indices.emplace_back(offset + index + 1);
            indices.emplace_back(center_index);
        }
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeTorusMesh(float p_radius,
                            float p_tube_radius,
                            int p_sectors,
                            int p_tube_sectors) {
    MeshComponent mesh;

    constexpr float two_pi = 2.0f * glm::pi<float>();
    for (int index_1 = 0; index_1 <= p_sectors; ++index_1) {
        const float angle_1 = two_pi * index_1 / p_sectors;
        for (int index_2 = 0; index_2 <= p_tube_sectors; ++index_2) {
            const float angle_2 = two_pi * index_2 / p_tube_sectors;
            const float x = (p_radius + p_tube_radius * glm::cos(angle_2)) * glm::cos(angle_1);
            const float y = p_tube_radius * glm::sin(angle_2);
            const float z = (p_radius + p_tube_radius * glm::cos(angle_2)) * glm::sin(angle_1);
            const float nx = p_tube_radius * glm::cos(angle_2) * glm::cos(angle_1);
            const float ny = p_tube_radius * glm::sin(angle_2);
            const float nz = p_tube_radius * glm::cos(angle_2) * glm::sin(angle_1);

            mesh.positions.emplace_back(Vector3f(x, y, z));
            mesh.normals.emplace_back(normalize(Vector3f(nx, ny, nz)));
            mesh.texcoords_0.emplace_back(Vector2f());
        }
    }

    auto& indices = mesh.indices;

    for (int index_1 = 0; index_1 < p_sectors; ++index_1) {
        for (int index_2 = 0; index_2 < p_tube_sectors; ++index_2) {
            /* a - b
               |   |
               c - d */
            const uint32_t a = index_2 + index_1 * (p_tube_sectors + 1);
            const uint32_t b = a + (p_tube_sectors + 1);
            const uint32_t c = a + 1;
            const uint32_t d = b + 1;

            indices.emplace_back(a);
            indices.emplace_back(c);
            indices.emplace_back(b);

            indices.emplace_back(b);
            indices.emplace_back(c);
            indices.emplace_back(d);
        }
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

void BoxWireFrameHelper(const Vector3f& p_min,
                        const Vector3f& p_max,
                        std::vector<Vector3f>& p_out_positions,
                        std::vector<uint32_t>& p_out_indices) {
    p_out_positions = {
        Vector3f(p_min.x, p_max.y, p_max.z),  // A
        Vector3f(p_min.x, p_min.y, p_max.z),  // B
        Vector3f(p_max.x, p_min.y, p_max.z),  // C
        Vector3f(p_max.x, p_max.y, p_max.z),  // D
        Vector3f(p_min.x, p_max.y, p_min.z),  // E
        Vector3f(p_min.x, p_min.y, p_min.z),  // F
        Vector3f(p_max.x, p_min.y, p_min.z),  // G
        Vector3f(p_max.x, p_max.y, p_min.z),  // H
    };

    p_out_indices = {
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
}

MeshComponent MakeBoxMesh(float size) {
    MeshComponent mesh;
    Vector3f min(-size);
    Vector3f max(+size);
    BoxWireFrameHelper(min, max, mesh.positions, mesh.indices);
    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeGrassBillboard(const Vector3f& p_scale) {
    MeshComponent mesh;

    const float x = p_scale.x;
    const float y = p_scale.y;

    std::array<Vector4f, 4> points = {
        Vector4f(-x, 2 * y, 0.0f, 1.0f),  // A
        Vector4f(-x, 0.0f, 0.0f, 1.0f),   // B
        Vector4f(+x, 0.0f, 0.0f, 1.0f),   // C
        Vector4f(+x, 2 * y, 0.0f, 1.0f),  // D
    };

    // @TODO: correct sampler
    constexpr std::array<Vector2f, 4> uvs = {
        Vector2f(0, 1),  // top-left
        Vector2f(0, 0),  // bottom-left
        Vector2f(1, 0),  // bottom-right
        Vector2f(1, 1),  // top-right
    };

    constexpr uint32_t indices[] = {
        A, B, D,  // ABD
        D, B, C,  // DBC
    };

    Degree angle;
    for (int i = 0; i < 3; ++i, angle += Degree(120.0f)) {
        const Matrix4x4f rotation = Rotate(angle, Vector3f(0, 1, 0));
        const Vector4f normal4 = rotation * Vector4f{ 0, 0, 1, 0 };
        const Vector3f normal = normal4.xyz;

        uint32_t offset = static_cast<uint32_t>(mesh.positions.size());
        for (size_t j = 0; j < points.size(); ++j) {
            Vector4f tmp = rotation * points[i];
            mesh.positions.emplace_back(Vector3f(tmp.xyz));
            mesh.normals.emplace_back(normal);
            mesh.texcoords_0.emplace_back(uvs[j]);
        }

        for (int j = 0; j < array_length(indices); ++j) {
            mesh.indices.emplace_back(indices[j] + offset);
        }
    }

    // flip uv
    for (auto& uv : mesh.texcoords_0) {
        uv.y = 1.0f - uv.y;
    }

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(mesh.indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

MeshComponent MakeSkyBoxMesh() {
    float size = 1.0f;
    MeshComponent mesh;
    mesh.positions = {
        Vector3f(-size, +size, +size),  // A
        Vector3f(-size, -size, +size),  // B
        Vector3f(+size, -size, +size),  // C
        Vector3f(+size, +size, +size),  // D
        Vector3f(-size, +size, -size),  // E
        Vector3f(-size, -size, -size),  // F
        Vector3f(+size, -size, -size),  // G
        Vector3f(+size, +size, -size),  // H
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

    MeshComponent::MeshSubset subset;
    subset.index_count = static_cast<uint32_t>(mesh.indices.size());
    subset.index_offset = 0;
    mesh.subsets.emplace_back(subset);

    mesh.CreateRenderData();
    return mesh;
}

// load scene
MeshComponent MakeBoxWireframeMesh(float size) {
    MeshComponent mesh;
    mesh.positions = {
        Vector3f(-size, +size, +size),  // A
        Vector3f(-size, -size, +size),  // B
        Vector3f(+size, -size, +size),  // C
        Vector3f(+size, +size, +size),  // D
        Vector3f(-size, +size, -size),  // E
        Vector3f(-size, -size, -size),  // F
        Vector3f(+size, -size, -size),  // G
        Vector3f(+size, +size, -size),  // H
    };

    mesh.indices = { A, B, B, C, C, D, D, A, E, F, F, G, G, H, H, E, A, E, B, F, D, H, C, G };

    return mesh;
}

}  // namespace my
