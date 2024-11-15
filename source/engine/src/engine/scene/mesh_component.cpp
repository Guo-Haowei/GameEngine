#include "mesh_component.h"

namespace my {

static size_t get_stride(MeshComponent::VertexAttribute::NAME p_name) {
    switch (p_name) {
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
void vertex_attrib(MeshComponent::VertexAttribute& p_attrib, const std::vector<T>& p_buffer, size_t& p_in_out_offset) {
    p_attrib.offset_in_byte = (uint32_t)p_in_out_offset;
    p_attrib.size_in_byte = (uint32_t)(math::Align(sizeof(T) * p_buffer.size(), size_t(16)));
    p_attrib.stride = (uint32_t)get_stride(p_attrib.name);
    p_in_out_offset += p_attrib.size_in_byte;
}

void MeshComponent::CreateRenderData() {
    // @HACK: fill dummy textures
#if 0
    if (texcoords_0.empty()) {
        for (size_t i = 0; i < positions.size(); ++i) {
            texcoords_0.emplace_back(vec2(0, 0));
        }
    }

    DEV_ASSERT(texcoords_0.size());
    DEV_ASSERT(normals.size());
#endif
    // AABB
    localBound.MakeInvalid();
    for (MeshSubset& subset : subsets) {
        subset.local_bound.MakeInvalid();
        for (uint32_t i = 0; i < subset.index_count; ++i) {
            const vec3& point = positions[indices[i + subset.index_offset]];
            subset.local_bound.ExpandPoint(point);
        }
        subset.local_bound.MakeValid();
        localBound.UnionBox(subset.local_bound);
    }
    // Attributes
    for (int i = 0; i < VertexAttribute::COUNT; ++i) {
        attributes[i].name = static_cast<VertexAttribute::NAME>(i);
    }

    vertexBufferSize = 0;
    vertex_attrib(attributes[VertexAttribute::POSITION], positions, vertexBufferSize);
    vertex_attrib(attributes[VertexAttribute::NORMAL], normals, vertexBufferSize);
    vertex_attrib(attributes[VertexAttribute::TEXCOORD_0], texcoords_0, vertexBufferSize);
    vertex_attrib(attributes[VertexAttribute::TEXCOORD_1], texcoords_1, vertexBufferSize);
    vertex_attrib(attributes[VertexAttribute::TANGENT], tangents, vertexBufferSize);
    vertex_attrib(attributes[VertexAttribute::JOINTS_0], joints_0, vertexBufferSize);
    vertex_attrib(attributes[VertexAttribute::WEIGHTS_0], weights_0, vertexBufferSize);
    vertex_attrib(attributes[VertexAttribute::COLOR_0], color_0, vertexBufferSize);
    return;
}

std::vector<char> MeshComponent::GenerateCombinedBuffer() const {
    std::vector<char> result;
    result.resize(vertexBufferSize);

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

void MeshComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << flags;
        p_archive << indices;
        p_archive << positions;
        p_archive << normals;
        p_archive << tangents;
        p_archive << texcoords_0;
        p_archive << texcoords_1;
        p_archive << joints_0;
        p_archive << weights_0;
        p_archive << color_0;
        p_archive << subsets;
        p_archive << armatureId;
    } else {
        p_archive >> flags;
        p_archive >> indices;
        p_archive >> positions;
        p_archive >> normals;
        p_archive >> tangents;
        p_archive >> texcoords_0;
        p_archive >> texcoords_1;
        p_archive >> joints_0;
        p_archive >> weights_0;
        p_archive >> color_0;
        p_archive >> subsets;
        p_archive >> armatureId;

        CreateRenderData();
    }
}

}  // namespace my
