#pragma once
#include "assets/loader.h"
#include "scene/scene.h"

struct aiMesh;
struct aiNode;
struct aiMaterial;
struct aiScene;
struct aiAnimation;

namespace my {

class LoaderAssimp : public Loader<Scene> {
public:
    LoaderAssimp(const std::string& p_path) : Loader<Scene>{ p_path } {}

    static std::shared_ptr<Loader<Scene>> Create(const std::string& p_path) {
        return std::make_shared<LoaderAssimp>(p_path);
    }

    bool Load(Scene* p_data) override;

protected:
    void ProcessMaterial(aiMaterial& p_material);
    void ProcessMesh(const aiMesh& p_mesh);

    ecs::Entity ProcessNode(const aiNode* p_node, ecs::Entity p_parent);

    std::vector<ecs::Entity> m_materials;
    std::vector<ecs::Entity> m_meshes;

    Scene* m_scene;
};

}  // namespace my
