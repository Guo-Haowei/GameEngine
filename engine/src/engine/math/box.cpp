#include "box.h"

#include "detail/vector3.h"

namespace my {

template<>
float Box<3>::SurfaceArea() const {
    if (!IsValid()) {
        return 0.0f;
    }
    Vector<float, 3> span(abs(Size()));
    const float result = 2.0f * (span.x * span.y +
                                 span.x * span.z +
                                 span.y * span.z);
    return result;
}

}  // namespace my
