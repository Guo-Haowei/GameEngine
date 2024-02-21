#pragma once
#include "assets/asset_handle.h"
#include "core/base/rid.h"
#include "core/math/aabb.h"
#include "core/math/angle.h"
#include "core/systems/entity.h"
#include "shader_constants.h"

namespace my {

class Archive;
class Scene;

//--------------------------------------------------------------------------------------------------
// Hierarchy Component
//--------------------------------------------------------------------------------------------------
class HierarchyComponent {
public:
    ecs::Entity GetParent() const { return m_parent_id; }

    void serialize(Archive& archive, uint32_t version);

private:
    ecs::Entity m_parent_id;

    // Non-serialized attributes

    friend class Scene;
};

//--------------------------------------------------------------------------------------------------
// Mesh Component
//--------------------------------------------------------------------------------------------------
struct MeshComponent {
    enum : uint32_t {
        NONE = BIT(0),
        RENDERABLE = BIT(1),
        DOUBLE_SIDED = BIT(2),
        DYNAMIC = BIT(3),
    };

    struct VertexAttribute {
        enum NAME {
            POSITION = 0,
            NORMAL,
            TEXCOORD_0,
            TEXCOORD_1,
            TANGENT,
            JOINTS_0,
            WEIGHTS_0,
            COLOR_0,
            COUNT,
        } name;

        uint32_t offset_in_byte = 0;
        uint32_t size_in_byte = 0;
        uint32_t stride = 0;

        bool is_valid() const { return size_in_byte != 0; }
    };

    uint32_t flags = RENDERABLE;
    std::vector<uint32_t> indices;
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec3> tangents;
    std::vector<vec2> texcoords_0;
    std::vector<vec2> texcoords_1;
    std::vector<ivec4> joints_0;
    std::vector<vec4> weights_0;
    std::vector<vec3> color_0;

    struct MeshSubset {
        ecs::Entity material_id;
        uint32_t index_offset = 0;
        uint32_t index_count = 0;
        AABB local_bound;
    };
    std::vector<MeshSubset> subsets;

    ecs::Entity armature_id;

    // Non-serialized
    mutable RID gpu_resource;
    AABB local_bound;

    VertexAttribute attributes[VertexAttribute::COUNT];
    size_t vertex_buffer_size = 0;  // combine vertex buffer

    void create_render_data();
    std::vector<char> generate_combined_buffer() const;

    void serialize(Archive& archive, uint32_t version);
};

//--------------------------------------------------------------------------------------------------
// Light Component
//--------------------------------------------------------------------------------------------------
struct LightComponent {
    enum : uint32_t {
        NONE = BIT(0),
        DIRTY = BIT(1),
        CAST_SHADOW = BIT(2),
    };

    bool cast_shadow() const { return flags & CAST_SHADOW; }

    uint32_t flags = NONE;
    int type = LIGHT_TYPE_OMNI;
    vec3 color = vec3(1);
    float energy = 10.0f;

    // Non-serialized
    float max_distance;

    // @TODO: serialize
    // @TODO: edit
    struct {
        float constant;
        float linear;
        float quadratic;
    } atten;

    void serialize(Archive& archive, uint32_t version);
};

//--------------------------------------------------------------------------------------------------
// Object Compoinent
//--------------------------------------------------------------------------------------------------
struct ObjectComponent {
    enum : uint32_t {
        NONE = BIT(0),
        RENDERABLE = BIT(1),
        CAST_SHADOW = BIT(2),
    };

    uint32_t flags = RENDERABLE | CAST_SHADOW;

    /// mesh
    ecs::Entity mesh_id;

    void serialize(Archive& archive, uint32_t version);
};

//--------------------------------------------------------------------------------------------------
// Animation Component
//--------------------------------------------------------------------------------------------------
struct AnimationComponent {
    enum : uint32_t {
        NONE = 0,
        PLAYING = 1 << 0,
        LOOPED = 1 << 1,
    };

    struct Channel {
        enum Path {
            PATH_TRANSLATION,
            PATH_ROTATION,
            PATH_SCALE,

            PATH_UNKNOWN,
        };

        Path path = PATH_UNKNOWN;
        ecs::Entity target_id;
        int sampler_index = -1;
    };
    struct Sampler {
        std::vector<float> keyframe_times;
        std::vector<float> keyframe_data;
    };

    bool is_playing() const { return flags & PLAYING; }
    bool is_looped() const { return flags & LOOPED; }
    float get_legnth() const { return end - start; }
    float is_end() const { return timer > end; }

    uint32_t flags = LOOPED;
    float start = 0;
    float end = 0;
    float timer = 0;
    float amount = 1;  // blend amount
    float speed = 1;

    std::vector<Channel> channels;
    std::vector<Sampler> samplers;

    void serialize(Archive& archive, uint32_t version);
};

//--------------------------------------------------------------------------------------------------
// Armature Component
//--------------------------------------------------------------------------------------------------
struct ArmatureComponent {
    enum FLAGS {
        NONE = 0,
    };
    uint32_t flags = NONE;

    std::vector<ecs::Entity> bone_collection;
    std::vector<mat4> inverse_bind_matrices;

    // Non-Serialized
    std::vector<mat4> bone_transforms;

    void serialize(Archive& archive, uint32_t version);
};

//--------------------------------------------------------------------------------------------------
// Rigid Body Component
//--------------------------------------------------------------------------------------------------
struct RigidBodyComponent {
    enum CollisionShape {
        SHAPE_UNKNOWN,
        SHAPE_SPHERE,
        SHAPE_BOX,
        SHAPE_MAX,
    };

    union Parameter {
        struct {
            vec3 half_size;
        } box;
        struct {
            float radius;
        } sphere;
    };

    float mass = 1.0f;
    CollisionShape shape;
    Parameter param;

    void serialize(Archive& archive, uint32_t version);
};

}  // namespace my