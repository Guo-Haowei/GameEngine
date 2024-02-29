#include "core/framework/asset_manager.h"
#include "core/io/archive.h"
#include "scene_components.h"

namespace my {

void HierarchyComponent::serialize(Archive& archive, uint32_t) {
    m_parent_id.serialize(archive);
}

void ObjectComponent::serialize(Archive& archive, uint32_t) {
    mesh_id.serialize(archive);
}

void AnimationComponent::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << flags;
        archive << start;
        archive << end;
        archive << timer;
        archive << amount;
        archive << speed;
        archive << channels;

        uint64_t num_samplers = samplers.size();
        archive << num_samplers;
        for (uint64_t i = 0; i < num_samplers; ++i) {
            archive << samplers[i].keyframe_times;
            archive << samplers[i].keyframe_data;
        }
    } else {
        archive >> flags;
        archive >> start;
        archive >> end;
        archive >> timer;
        archive >> amount;
        archive >> speed;
        archive >> channels;

        uint64_t num_samplers = 0;
        archive >> num_samplers;
        samplers.resize(num_samplers);
        for (uint64_t i = 0; i < num_samplers; ++i) {
            archive >> samplers[i].keyframe_times;
            archive >> samplers[i].keyframe_data;
        }
    }
}

void ArmatureComponent::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << flags;
        archive << bone_collection;
        archive << inverse_bind_matrices;
    } else {
        archive >> flags;
        archive >> bone_collection;
        archive >> inverse_bind_matrices;
    }
}

void RigidBodyComponent::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << shape;
        archive << param;
        archive << mass;
    } else {
        archive >> shape;
        archive >> param;
        archive >> mass;
    }
}

}  // namespace my
