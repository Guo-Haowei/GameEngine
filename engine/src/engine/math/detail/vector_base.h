#pragma once
#include "forward.h"

namespace my::math {

template<Arithmetic T, int N>
    requires(N >= 2 && N <= 4)
struct VectorBase {
    using Self = VectorBase<T, N>;

    constexpr T* Data() { return reinterpret_cast<T*>(this); }
    constexpr const T* Data() const { return reinterpret_cast<const T*>(this); }

    constexpr void Set(const T* p_data) {
        T* data = Data();
        data[0] = p_data[0];
        data[1] = p_data[1];
        if constexpr (N >= 3) {
            data[2] = p_data[2];
        }
        if constexpr (N >= 4) {
            data[3] = p_data[3];
        }
    }

    constexpr T& operator[](int p_index) {
        return Data()[p_index];
    }

    constexpr const T& operator[](int p_index) const {
        return Data()[p_index];
    }
};

}  // namespace my::math
