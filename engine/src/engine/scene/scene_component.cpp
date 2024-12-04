#include "scene_component.h"

#include "engine/core/framework/asset_registry.h"
#include "engine/core/io/archive.h"

namespace my {

#pragma region NAME_COMPONENT
void NameComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << m_name;
    } else {
        p_archive >> m_name;
    }
}
#pragma endregion NAME_COMPONENT

#pragma region ANIMATION_COMPONENT
void AnimationComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << flags;
        p_archive << start;
        p_archive << end;
        p_archive << timer;
        p_archive << amount;
        p_archive << speed;
        p_archive << channels;

        uint64_t num_samplers = samplers.size();
        p_archive << num_samplers;
        for (uint64_t i = 0; i < num_samplers; ++i) {
            p_archive << samplers[i].keyframeTmes;
            p_archive << samplers[i].keyframeData;
        }
    } else {
        p_archive >> flags;
        p_archive >> start;
        p_archive >> end;
        p_archive >> timer;
        p_archive >> amount;
        p_archive >> speed;
        p_archive >> channels;

        uint64_t num_samplers = 0;
        p_archive >> num_samplers;
        samplers.resize(num_samplers);
        for (uint64_t i = 0; i < num_samplers; ++i) {
            p_archive >> samplers[i].keyframeTmes;
            p_archive >> samplers[i].keyframeData;
        }
    }
}
#pragma endregion ANIMATION_COMPONENT

#pragma region ARMATURE_COMPONENT
void ArmatureComponent::Serialize(Archive& p_archive, uint32_t) {
    if (p_archive.IsWriteMode()) {
        p_archive << flags;
        p_archive << boneCollection;
        p_archive << inverseBindMatrices;
    } else {
        p_archive >> flags;
        p_archive >> boneCollection;
        p_archive >> inverseBindMatrices;
    }
}
#pragma endregion ARMATURE_COMPONENT

#pragma region SCRIPT_COMPONENT
std::string& ScriptComponent::GetScriptRef() {
    return m_path;
}

void ScriptComponent::SetScript(const std::string& p_path) {
    if (p_path.empty()) {
        return;
    }

    if (p_path == m_path) {
        return;
    }

    AssetRegistry::GetSingleton().RequestAssetAsync(
        p_path, [](IAsset* p_asset, void* p_userdata) {
            auto source = dynamic_cast<TextAsset*>(p_asset);
            if (DEV_VERIFY(source)) {
                reinterpret_cast<ScriptComponent*>(p_userdata)->m_asset = source;
            }
        },
        this);

    m_path = p_path;
}

const char* ScriptComponent::GetSource() const {
    return m_asset ? m_asset->source.c_str() : nullptr;
}

void ScriptComponent::Serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.IsWriteMode()) {
        p_archive << m_path;
    } else {
        std::string path;
        p_archive >> path;
        SetScript(path);
    }
}
#pragma endregion SCRIPT_COMPONENT

}  // namespace my
