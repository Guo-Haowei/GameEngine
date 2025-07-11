#pragma once
#include "engine/core/base/concurrent_queue.h"
#include "engine/core/base/singleton.h"
#include "engine/runtime/module.h"

namespace my {

class Scene;
class Application;

class SceneManager : public Singleton<SceneManager>, public Module {
public:
    SceneManager()
        : Module("SceneManager") {}

    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;
    void Update();

    void RequestScene(std::string_view p_path);

    uint32_t GetRevision() const { return m_revision; }
    void BumpRevision() { ++m_revision; }

    void EnqueueSceneLoadingTask(Scene* p_scene, bool p_replace);

    Scene* GetScenePtr() const { return m_scene; }

private:
    bool TrySwapScene();

    Scene* m_scene = nullptr;

    uint32_t m_revision = 0;
    uint32_t m_lastRevision = 0;

    struct LoadSceneTask {
        bool replace;
        Scene* scene;
    };

    ConcurrentQueue<LoadSceneTask> m_loadingQueue;
};

}  // namespace my
