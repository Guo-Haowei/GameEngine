#include "core/framework/asset_manager.h"
#include "core/io/archive.h"
#include "scene_components.h"

namespace my {

void TransformComponent::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << m_flags;
        archive << m_scale;
        archive << m_translation;
        archive << m_rotation;
    } else {
        archive >> m_flags;
        archive >> m_scale;
        archive >> m_translation;
        archive >> m_rotation;
        set_dirty();
    }
}

void HierarchyComponent::serialize(Archive& archive, uint32_t) {
    m_parent_id.serialize(archive);
}

void MeshComponent::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << flags;
        archive << indices;
        archive << positions;
        archive << normals;
        archive << tangents;
        archive << texcoords_0;
        archive << texcoords_1;
        archive << joints_0;
        archive << weights_0;
        archive << color_0;
        archive << subsets;
        archive << armature_id;
    } else {
        archive >> flags;
        archive >> indices;
        archive >> positions;
        archive >> normals;
        archive >> tangents;
        archive >> texcoords_0;
        archive >> texcoords_1;
        archive >> joints_0;
        archive >> weights_0;
        archive >> color_0;
        archive >> subsets;
        archive >> armature_id;

        create_render_data();
    }
}

void MaterialComponent::serialize(Archive& archive, uint32_t) {
    if (archive.is_write_mode()) {
        archive << metallic;
        archive << roughness;
        archive << base_color;
        for (int i = 0; i < TEXTURE_MAX; ++i) {
            archive << textures[i].path;
        }
    } else {
        archive >> metallic;
        archive >> roughness;
        archive >> base_color;
        for (int i = 0; i < TEXTURE_MAX; ++i) {
            std::string path;
            archive >> path;
            request_image((TextureSlot)i, path);
        }
    }

    // @TODO: request image
}

void LightComponent::serialize(Archive& archive, uint32_t version) {
    unused(version);
    if (archive.is_write_mode()) {
        archive << flags;
        archive << type;
        archive << color;
        archive << energy;
        archive << atten.constant;
        archive << atten.linear;
        archive << atten.quadratic;
    } else {
        if (version >= 4) {
            archive >> flags;
        }
        archive >> type;
        archive >> color;
        archive >> energy;
        archive >> atten.constant;
        archive >> atten.linear;
        archive >> atten.quadratic;
    }
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
