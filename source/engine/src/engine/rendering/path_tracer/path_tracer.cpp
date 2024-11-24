#include "path_tracer.h"

#include "structured_buffer.hlsl.h"

namespace my {

gpu_bvh_t::gpu_bvh_t()
    : missIdx(-1), hitIdx(-1), leaf(0), geomIdx(-1) {
    _padding_0 = 0;
    _padding_1 = 0;
}

gpu_geometry_t::gpu_geometry_t()
    : kind(Kind::Invalid), materialId(-1) {}

gpu_geometry_t::gpu_geometry_t(const Vector3f& A, const Vector3f& B, const Vector3f& C, int material)
    : A(A), B(B), C(C), materialId(material) {
    kind = Kind::Triangle;
    CalcNormal();
}

gpu_geometry_t::gpu_geometry_t(const Vector3f& center, float radius, int material)
    : A(center), materialId(material) {
    // TODO: refactor
    kind = Kind::Sphere;
    this->radius = glm::max(0.01f, glm::abs(radius));
}

void gpu_geometry_t::CalcNormal() {
    using glm::cross;
    using glm::normalize;
    Vector3f BA = normalize(B - A);
    Vector3f CA = normalize(C - A);
    Vector3f norm = normalize(cross(BA, CA));
    normal1 = norm;
    normal2 = norm;
    normal3 = norm;
}

Vector3f gpu_geometry_t::Centroid() const {
    switch (kind) {
        case Kind::Triangle:
            return (A + B + C) / 3.0f;
        case Kind::Sphere:
            return A;
        default:
            assert(0);
            return Vector3f(0.0f);
    }
}

}  // namespace my
