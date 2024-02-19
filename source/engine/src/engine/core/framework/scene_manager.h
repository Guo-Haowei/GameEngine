#pragma once
#include "core/base/singleton.h"
#include "core/framework/module.h"
#include "scene/scene.h"

namespace my {

class Application;

class SceneManager : public Singleton<SceneManager>, public Module {
public:
    SceneManager() : Module("SceneManager") {}

    bool initialize() override;
    void finalize() override;
    void update(float dt);

    void request_scene(std::string_view path);

    void set_loading_scene(Scene* scene) {
        m_loading_scene.store(scene);
    }

    uint32_t get_revision() const { return m_revision; }
    void bump_revision() { ++m_revision; }

    static Scene& get_scene();

private:
    bool try_swap_scene();

    Scene* m_scene = nullptr;
    std::atomic<Scene*> m_loading_scene = nullptr;

    uint32_t m_revision = 0;
    uint32_t m_last_revision = 0;
};

}  // namespace my
