#include "scene_importer.h"

namespace my {

SceneImporter::SceneImporter(Scene& scene, const std::string& file_path, const char* loader_name)
    : m_scene(scene), m_file_path(file_path), m_loader_name(loader_name) {

    std::filesystem::path system_path{ file_path };
    m_scene_name = system_path.filename().string();
    m_search_path = system_path.remove_filename().string();
}

auto SceneImporter::import() -> std::expected<void, std::string> {
    if (!import_impl()) {
        return std::unexpected(std::format("[{}] {}.", m_loader_name, m_error));
    }

    //// process images
    // LOG_VERBOSE("[{}] loading {} images...", m_loader_name, m_scene.get_count<MaterialComponent>());
    // for (const MaterialComponent& material : m_scene.get_component_array<MaterialComponent>()) {
    //     for (int i = 0; i < array_length(material.textures); ++i) {
    //         const std::string& image_path = material.textures[i].name;
    //         if (!image_path.empty()) {
    //             AssetManager::singleton().load_image_async(image_path);
    //         }
    //     }
    // }

    LOG_VERBOSE("[{}] generating bounding boxes", m_loader_name);
    // process meshes
    for (MeshComponent& mesh : m_scene.get_component_array<MeshComponent>()) {
        mesh.create_render_data();
    }

    m_scene.update(0.0f);
    return std::expected<void, std::string>();
}

}  // namespace my
