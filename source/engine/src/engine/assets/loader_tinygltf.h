#include "loader.h"

namespace tinygltf {
class Model;
struct Animation;
struct Mesh;
}  // namespace tinygltf

namespace my {

class LoaderTinyGLTF : public Loader<Scene> {
public:
    LoaderTinyGLTF(const std::string& path) : Loader<Scene>{ path } {}

    bool load(Scene* data) override;

    static std::shared_ptr<Loader<Scene>> create(const std::string& path) {
        return std::make_shared<LoaderTinyGLTF>(path);
    }

protected:
    void process_node(int node_index, ecs::Entity parent);
    void process_mesh(const tinygltf::Mesh& gltf_mesh, int id);
    void process_animation(const tinygltf::Animation& gltf_anim, int id);

    std::unordered_map<int, ecs::Entity> m_entity_map;
    std::shared_ptr<tinygltf::Model> m_model;
    Scene* m_scene;
};

}  // namespace my
