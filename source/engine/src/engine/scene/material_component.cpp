#include "material_component.h"

#include "core/framework/asset_manager.h"
#include "core/io/archive.h"

namespace my {

void MaterialComponent::request_image(int p_slot, const std::string& p_path) {
    if (!p_path.empty()) {
        textures[p_slot].path = p_path;
        textures[p_slot].image = AssetManager::singleton().load_image_async(p_path);
    }
}

void MaterialComponent::serialize(Archive& p_archive, uint32_t p_version) {
    unused(p_version);

    if (p_archive.is_write_mode()) {
        p_archive << metallic;
        p_archive << roughness;
        p_archive << base_color;
        for (int i = 0; i < TEXTURE_MAX; ++i) {
            p_archive << textures[i].enabled;
            p_archive << textures[i].path;
        }
    } else {
        p_archive >> metallic;
        p_archive >> roughness;
        p_archive >> base_color;
        for (int i = 0; i < TEXTURE_MAX; ++i) {
            if (p_version >= 7) {
                p_archive >> textures[i].enabled;
            }
            std::string& path = textures[i].path;
            p_archive >> path;

            // request image
            if (!path.empty()) {
                textures[i].image = AssetManager::singleton().load_image_async(path);
            }
        }
    }
}

}  // namespace my
