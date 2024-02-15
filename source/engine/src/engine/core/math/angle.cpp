#pragma once
#include "angle.h"

namespace vct {

Degree& Degree::operator=(const Radians& rad) {
    m_value = rad.to_degree();
    return *this;
}

Radians& Radians::operator=(const Degree& degree) {
    m_value = degree.to_rad();
    return *this;
}

}  // namespace vct
