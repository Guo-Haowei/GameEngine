#include "layer.h"

#include "engine/core/framework/application.h"
#include "engine/core/framework/script_manager.h"

namespace my {

void GameLayer::OnAttach() {
    LOG("GameLayer '{}' attached", m_name);

    Scene* scene = m_app->GetActiveScene();
    if (DEV_VERIFY(scene)) {
        m_app->GetScriptManager()->OnSimBegin(*scene);
    }

    OnAttachInternal();
}

void GameLayer::OnDetach() {
    LOG("GameLayer '{}' detached", m_name);

    OnDetachInternal();

    Scene* scene = m_app->GetActiveScene();
    if (DEV_VERIFY(scene)) {
        m_app->GetScriptManager()->OnSimEnd(*scene);
    }
}

}  // namespace my
