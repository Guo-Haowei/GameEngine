#include "box.h"

namespace my {

template<>
float Box<3>::SurfaceArea() const {
    if (!IsValid()) {
        return 0.0f;
    }
    NewVector3f span = math::Abs(Size());
    const float result = 2.0f * (span.x * span.y +
                                 span.x * span.z +
                                 span.y * span.z);
    return result;
}

}  // namespace my
