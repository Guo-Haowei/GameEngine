#pragma once

#include "engine/core/io/archive.h"
#include "engine/core/math/geomath.h"
#include "engine/renderer/gpu_resource.h"

namespace my {

struct AssetHandle {
    constexpr AssetHandle() : hash(0) {}

    AssetHandle(const std::string& p_path) {
#if USING(DEBUG_BUILD)
        path = p_path;
#endif
        hash = std::hash<std::string>()(p_path);
    }

    bool operator==(const AssetHandle& p_rhs) const {
        return hash == p_rhs.hash;
    }

    bool operator!=(const AssetHandle& p_rhs) const {
        return hash == p_rhs.hash;
    }

    bool operator<(const AssetHandle& p_rhs) const {
        return hash < p_rhs.hash;
    }

    bool operator<=(const AssetHandle& p_rhs) const {
        return hash <= p_rhs.hash;
    }

    bool operator>(const AssetHandle& p_rhs) const {
        return hash > p_rhs.hash;
    }

    bool operator>=(const AssetHandle& p_rhs) const {
        return hash >= p_rhs.hash;
    }

    size_t hash;
#if USING(DEBUG_BUILD)
    std::string path;
#endif
};

}  // namespace my

namespace std {

template<>
struct hash<my::AssetHandle> {
    std::size_t operator()(const my::AssetHandle& p_handle) const {
        return p_handle.hash;
    }
};

}  // namespace std
