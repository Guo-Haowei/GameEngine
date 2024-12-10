#pragma once
#include "engine/scene/scene.h"

namespace my {

class ScriptableEntity {
public:
    virtual ~ScriptableEntity() = default;

    template<typename T>
    T* GetComponent() {
        return m_scene->GetComponent<T>(m_id);
    }

    virtual void OnCollision(ecs::Entity) {}

protected:
    virtual void OnCreate() {}
    virtual void OnDestroy() {}
    virtual void OnUpdate(float) {}

    ecs::Entity m_id;
    Scene* m_scene;

    friend class ScriptManager;
};

}  // namespace my
