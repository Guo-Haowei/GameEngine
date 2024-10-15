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

    void requestScene(std::string_view p_path);

    uint32_t getRevision() const { return m_revision; }
    void bumpRevision() { ++m_revision; }

    void enqueueSceneLoadingTask(Scene* p_scene, bool p_replace);

    // @TODO: bad idea to make it globally accessible, fix it
    static Scene& getScene();

private:
    bool trySwapScene();

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
