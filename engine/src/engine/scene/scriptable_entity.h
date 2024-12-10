#pragma once
#include "engine/scene/scene.h"

namespace my {

class ScriptableEntity {
public:
    virtual ~ScriptableEntity() = default;

protected:
    virtual void OnCreate() {}
    virtual void OnDestroy() {}
    virtual void OnUpdate(float) {}

    ecs::Entity m_id;
    Scene* m_scene;

    friend class Scene;
};

}  // namespace my
