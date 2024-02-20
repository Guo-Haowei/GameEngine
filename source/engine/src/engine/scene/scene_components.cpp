#pragma once
#include "scene_components.h"

#include "core/framework/asset_manager.h"

namespace my {

//--------------------------------------------------------------------------------------------------
// Transform Component
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
// Mesh Component
//--------------------------------------------------------------------------------------------------
static size_t get_stride(MeshComponent::VertexAttribute::NAME name) {
    switch (name) {
        case MeshComponent::VertexAttribute::POSITION:
        case MeshComponent::VertexAttribute::NORMAL:
        case MeshComponent::VertexAttribute::TANGENT:
            return sizeof(vec3);
        case MeshComponent::VertexAttribute::TEXCOORD_0:
        case MeshComponent::VertexAttribute::TEXCOORD_1:
            return sizeof(vec2);
        case MeshComponent::VertexAttribute::JOINTS_0:
            return sizeof(ivec4);
        case MeshComponent::VertexAttribute::COLOR_0:
        case MeshComponent::VertexAttribute::WEIGHTS_0:
            return sizeof(vec4);
        default:
            CRASH_NOW();
            return 0;
    }
}

template<typename T>
void vertex_attrib(MeshComponent::VertexAttribute& attrib, const std::vector<T>& buffer, size_t& in_out_offset) {
    attrib.offset_in_byte = (uint32_t)in_out_offset;
    attrib.size_in_byte = (uint32_t)(math::align(sizeof(T) * buffer.size(), 16llu));
    attrib.stride = (uint32_t)get_stride(attrib.name);
    in_out_offset += attrib.size_in_byte;
}

void MeshComponent::create_render_data() {
    // @HACK: fill dummy textures
    if (texcoords_0.empty()) {
        for (size_t i = 0; i < normals.size(); ++i) {
            texcoords_0.emplace_back(vec2(0, 0));
        }
    }

    DEV_ASSERT(texcoords_0.size());
    DEV_ASSERT(normals.size());
    // AABB
    local_bound.make_invalid();
    for (MeshSubset& subset : subsets) {
        subset.local_bound.make_invalid();
        for (uint32_t i = 0; i < subset.index_count; ++i) {
            const vec3& point = positions[indices[i + subset.index_offset]];
            subset.local_bound.expand_point(point);
        }
        subset.local_bound.make_valid();
        local_bound.union_box(subset.local_bound);
    }
    // Attributes
    for (int i = 0; i < VertexAttribute::COUNT; ++i) {
        attributes[i].name = static_cast<VertexAttribute::NAME>(i);
    }

    vertex_buffer_size = 0;
    vertex_attrib(attributes[VertexAttribute::POSITION], positions, vertex_buffer_size);
    vertex_attrib(attributes[VertexAttribute::NORMAL], normals, vertex_buffer_size);
    vertex_attrib(attributes[VertexAttribute::TEXCOORD_0], texcoords_0, vertex_buffer_size);
    vertex_attrib(attributes[VertexAttribute::TEXCOORD_1], texcoords_1, vertex_buffer_size);
    vertex_attrib(attributes[VertexAttribute::TANGENT], tangents, vertex_buffer_size);
    vertex_attrib(attributes[VertexAttribute::JOINTS_0], joints_0, vertex_buffer_size);
    vertex_attrib(attributes[VertexAttribute::WEIGHTS_0], weights_0, vertex_buffer_size);
    vertex_attrib(attributes[VertexAttribute::COLOR_0], color_0, vertex_buffer_size);
    return;
}

std::vector<char> MeshComponent::generate_combined_buffer() const {
    std::vector<char> result;
    result.resize(vertex_buffer_size);

    auto safe_copy = [&](const VertexAttribute& attrib, const void* data) {
        if (attrib.size_in_byte == 0) {
            return;
        }

        memcpy(result.data() + attrib.offset_in_byte, data, attrib.size_in_byte);
        return;
    };
    safe_copy(attributes[VertexAttribute::POSITION], positions.data());
    safe_copy(attributes[VertexAttribute::NORMAL], normals.data());
    safe_copy(attributes[VertexAttribute::TEXCOORD_0], texcoords_0.data());
    safe_copy(attributes[VertexAttribute::TEXCOORD_1], texcoords_1.data());
    safe_copy(attributes[VertexAttribute::TANGENT], tangents.data());
    safe_copy(attributes[VertexAttribute::JOINTS_0], joints_0.data());
    safe_copy(attributes[VertexAttribute::WEIGHTS_0], weights_0.data());
    safe_copy(attributes[VertexAttribute::COLOR_0], color_0.data());
    return result;
}

//--------------------------------------------------------------------------------------------------
// Mesh Component
//--------------------------------------------------------------------------------------------------
void MaterialComponent::request_image(TextureSlot slot, const std::string& path) {
    if (!path.empty()) {
        textures[slot].path = path;
        textures[slot].image = AssetManager::singleton().load_image_async(path);
    }
}

}  // namespace my