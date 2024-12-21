#include "angle.h"

namespace my {

Degree& Degree::operator=(const Radian& p_rad) {
    m_value = p_rad.ToDegree();
    return *this;
}

Radian& Radian::operator=(const Degree& p_degree) {
    m_value = p_degree.GetRadians();
    return *this;
}

}  // namespace my
