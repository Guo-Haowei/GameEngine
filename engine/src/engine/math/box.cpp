#include "box.h"

#include "detail/vector3.h"

namespace my::math {

template<>
float Box<3>::SurfaceArea() const {
    if (!IsValid()) {
        return 0.0f;
    }
    Vector<float, 3> span(math::abs(Size()));
    const float result = 2.0f * (span.x * span.y +
                                 span.x * span.z +
                                 span.y * span.z);
    return result;
}

}  // namespace my::math
