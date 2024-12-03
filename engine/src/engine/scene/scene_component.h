#pragma once
#include "engine/core/math/geomath.h"
#include "engine/core/systems/entity.h"

namespace my {

struct TextAsset;
class Archive;

#pragma region NAME_COMPONENT
class NameComponent {
public:
    NameComponent() = default;

    NameComponent(const char* p_name) { m_name = p_name; }

    void SetName(const char* p_name) { m_name = p_name; }
    void SetName(const std::string& p_name) { m_name = p_name; }

    const std::string& GetName() const { return m_name; }
    std::string& GetNameRef() { return m_name; }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    std::string m_name;
};
#pragma endregion NAME_COMPONENT

#pragma region ANIMATION_COMPONENT
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
        ecs::Entity targetId;
        int samplerIndex = -1;
    };
    struct Sampler {
        std::vector<float> keyframeTmes;
        std::vector<float> keyframeData;
    };

    bool IsPlaying() const { return flags & PLAYING; }
    bool IsLooped() const { return flags & LOOPED; }
    float GetLegnth() const { return end - start; }
    float IsEnd() const { return timer > end; }

    uint32_t flags = LOOPED;
    float start = 0;
    float end = 0;
    float timer = 0;
    float amount = 1;  // blend amount
    float speed = 1;

    std::vector<Channel> channels;
    std::vector<Sampler> samplers;

    void Serialize(Archive& p_archive, uint32_t p_version);
};
#pragma endregion ANIMATION_COMPONENT

#pragma region ARMATURE_COMPONENT
struct ArmatureComponent {
    enum FLAGS {
        NONE = 0,
    };
    uint32_t flags = NONE;

    std::vector<ecs::Entity> boneCollection;
    std::vector<Matrix4x4f> inverseBindMatrices;

    // Non-Serialized
    std::vector<Matrix4x4f> boneTransforms;

    void Serialize(Archive& p_archive, uint32_t p_version);
};
#pragma endregion ARMATURE_COMPONENT

#pragma region SCRIPT_COMPONENT
struct ScriptComponent {
    std::string path;

    // Non-Serialized
    TextAsset* source{ nullptr };

    void Serialize(Archive& p_archive, uint32_t p_version);
};
#pragma endregion SCRIPT_COMPONENT

// #pragma region _COMPONENT
// #pragma endregion _COMPONENT

}  // namespace my
