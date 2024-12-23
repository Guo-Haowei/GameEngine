#pragma once

namespace my {

inline constexpr float OCEAN_RADIUS = 240.0f;
inline constexpr int CLOUD_COUNT = 20;
inline constexpr float WORLD_SPEED = 0.3f;
inline constexpr float MIN_HEIGHT = 15.f;
inline constexpr float MAX_HEIGHT = 45.f;
inline constexpr float AMP_WIDTH = 30.0f;
inline constexpr float AMP_HEIGHT = 30.0f;

enum : uint32_t {
    COLLISION_BIT_PLAYER = BIT(1),
    COLLISION_BIT_ROCK = BIT(2),
    COLLISION_BIT_BATTERY = BIT(3),
};

}  // namespace my
