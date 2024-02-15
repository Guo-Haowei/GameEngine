#pragma once
#include "core/io/archive.h"

namespace my::ecs {

class Entity {
public:
    static constexpr size_t INVALID_INDEX = ~0llu;
    static constexpr uint32_t INVALID_ID = 0;
    static constexpr uint32_t MAX_ID = ~0u;

    explicit constexpr Entity() : m_id(INVALID_ID) {}

    explicit constexpr Entity(uint32_t handle) : m_id(handle) {}

    ~Entity() = default;

    bool operator==(const Entity& rhs) const { return m_id == rhs.m_id; }

    bool operator!=(const Entity& rhs) const { return m_id != rhs.m_id; }

    bool is_valid() const { return m_id != INVALID_ID; }

    void make_invalid() { m_id = INVALID_ID; }

    constexpr uint32_t get_id() const { return m_id; }

    void serialize(Archive& archive);

    static Entity create();
    static uint32_t get_seed();
    static void set_seed(uint32_t seed);

    static const Entity INVALID;

private:
    uint32_t m_id;

    inline static std::atomic<uint32_t> s_id = MAX_ID;
};

}  // namespace my::ecs

namespace std {

template<>
struct hash<my::ecs::Entity> {
    std::size_t operator()(const my::ecs::Entity& entity) const { return std::hash<uint32_t>{}(entity.get_id()); }
};

template<>
struct less<my::ecs::Entity> {
    constexpr bool operator()(const my::ecs::Entity& lhs, const my::ecs::Entity& rhs) const {
        return lhs.get_id() < rhs.get_id();
    }
};

template<>
struct equal_to<my::ecs::Entity> {
    constexpr bool operator()(const my::ecs::Entity& lhs, const my::ecs::Entity& rhs) const {
        return lhs.get_id() == rhs.get_id();
    }
};

}  // namespace std
