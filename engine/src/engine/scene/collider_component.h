#pragma once
#include "engine/core/math/aabb.h"
#include "engine/systems/ecs/entity.h"

namespace YAML {
class Node;
class Emitter;
}  // namespace YAML

namespace my {

class Archive;

struct BoxColliderComponent {
    AABB box;

    void Serialize(Archive& p_archive, uint32_t p_version);
    WARNING_PUSH()
    WARNING_DISABLE(4100, "-Wunused-parameter")
    bool Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const { return true; }
    bool Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) { return true; }
    WARNING_POP()
};

struct MeshColliderComponent {
    ecs::Entity objectId;

    void Serialize(Archive& p_archive, uint32_t p_version);
    WARNING_PUSH()
    WARNING_DISABLE(4100, "-Wunused-parameter")
    bool Dump(YAML::Emitter& p_out, Archive& p_archive, uint32_t p_version) const { return true; }
    bool Undump(const YAML::Node& p_node, Archive& p_archive, uint32_t p_version) { return true; }
    WARNING_POP()
};

}  // namespace my
