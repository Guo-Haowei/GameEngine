#pragma once
#include "angle.h"

namespace my {

Degree& Degree::operator=(const Radians& p_rad) {
    m_value = p_rad.ToDegree();
    return *this;
}

Radians& Radians::operator=(const Degree& p_degree) {
    m_value = p_degree.ToRad();
    return *this;
}

}  // namespace my
