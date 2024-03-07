#pragma once
#include "core/base/concurrent_queue.h"
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

    void request_scene(std::string_view p_path);

    uint32_t get_revision() const { return m_revision; }
    void bump_revision() { ++m_revision; }

    void queue_loaded_scene(Scene* p_scene, bool p_replace);

    // @TODO: bad idea to make it globally accessible, fix it
    static Scene& get_scene();

private:
    bool try_swap_scene();

    Scene* m_scene = nullptr;

    uint32_t m_revision = 0;
    uint32_t m_last_revision = 0;

    struct LoadSceneTask {
        bool replace;
        Scene* scene;
    };

    ConcurrentQueue<LoadSceneTask> m_loading_queue;
};

}  // namespace my
