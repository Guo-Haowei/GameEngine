#pragma once
#include "core/io/archive.h"

namespace my::ecs {

class Entity {
public:
    static constexpr size_t INVALID_INDEX = ~0llu;
    static constexpr uint32_t INVALID_ID = 0;
    static constexpr uint32_t MAX_ID = ~0u;

    explicit constexpr Entity() : m_id(INVALID_ID) {}

    explicit constexpr Entity(uint32_t p_handle) : m_id(p_handle) {}

    ~Entity() = default;

    bool operator==(const Entity& p_rhs) const { return m_id == p_rhs.m_id; }

    bool operator!=(const Entity& p_rhs) const { return m_id != p_rhs.m_id; }

    bool isValid() const { return m_id != INVALID_ID; }

    void makeInvalid() { m_id = INVALID_ID; }

    constexpr uint32_t getId() const { return m_id; }

    void serialize(Archive& p_archive);

    static Entity create();
    static uint32_t getSeed();
    static void setSeed(uint32_t p_seed = INVALID_ID + 1);

    static const Entity INVALID;

private:
    uint32_t m_id;

    inline static std::atomic<uint32_t> s_id = MAX_ID;
};

}  // namespace my::ecs

namespace std {

template<>
struct hash<my::ecs::Entity> {
    std::size_t operator()(const my::ecs::Entity& p_entity) const { return std::hash<uint32_t>{}(p_entity.getId()); }
};

template<>
struct less<my::ecs::Entity> {
    constexpr bool operator()(const my::ecs::Entity& p_lhs, const my::ecs::Entity& p_rhs) const {
        return p_lhs.getId() < p_rhs.getId();
    }
};

template<>
struct equal_to<my::ecs::Entity> {
    constexpr bool operator()(const my::ecs::Entity& p_lhs, const my::ecs::Entity& p_rhs) const {
        return p_lhs.getId() == p_rhs.getId();
    }
};

}  // namespace std
