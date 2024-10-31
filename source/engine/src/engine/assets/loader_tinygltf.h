#include "assets/loader.h"
#include "scene/scene.h"

namespace tinygltf {
class Model;
struct Animation;
struct Mesh;
}  // namespace tinygltf

namespace my {

class LoaderTinyGLTF : public Loader<Scene> {
public:
    LoaderTinyGLTF(const std::string& p_path) : Loader<Scene>{ p_path } {}

    bool Load(Scene* p_data) override;

    static std::shared_ptr<Loader<Scene>> Create(const std::string& p_path) {
        return std::make_shared<LoaderTinyGLTF>(p_path);
    }

protected:
    void ProcessNode(int p_node_index, ecs::Entity p_parent);
    void ProcessMesh(const tinygltf::Mesh& p_gltf_mesh, int p_id);
    void ProcessAnimation(const tinygltf::Animation& p_gltf_anim, int p_id);

    std::unordered_map<int, ecs::Entity> m_entityMap;
    std::shared_ptr<tinygltf::Model> m_model;
    Scene* m_scene;
};

}  // namespace my
