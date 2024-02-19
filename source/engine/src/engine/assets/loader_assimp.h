#pragma once
#include "loader.h"

struct aiMesh;
struct aiNode;
struct aiMaterial;
struct aiScene;
struct aiAnimation;

namespace my {

class LoaderAssimp : public Loader<Scene> {
public:
    LoaderAssimp(const std::string& path) : Loader<Scene>{ path } {}

    static std::shared_ptr<Loader<Scene>> create(const std::string& path) {
        return std::make_shared<LoaderAssimp>(path);
    }

    bool load(Scene* data) override;

protected:
    void process_material(aiMaterial& material);
    void process_mesh(const aiMesh& mesh);

    ecs::Entity process_node(const aiNode* node, ecs::Entity parent);

    std::vector<ecs::Entity> m_materials;
    std::vector<ecs::Entity> m_meshes;

    Scene* m_scene;
};

}  // namespace my
