#include "entity.h"

#include "engine/core/io/archive.h"

namespace my::ecs {

const Entity Entity::INVALID{};

void Entity::Serialize(Archive& p_archive) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_id;
    } else {
        p_archive >> m_id;
    }
}

Entity Entity::Create() {
    CRASH_COND_MSG(s_id.load() == MAX_ID, "max number of entity allocated, did you forget to call setSeed()?");
    CRASH_COND_MSG(s_id.load() == 0, "seed id is 0, did you forget to call setSeed()?");
    Entity entity(s_id.fetch_add(1));
    return entity;
}

uint32_t Entity::GetSeed() {
    return s_id;
}

void Entity::SetSeed(uint32_t p_seed) {
    s_id = p_seed;
}

}  // namespace my::ecs
