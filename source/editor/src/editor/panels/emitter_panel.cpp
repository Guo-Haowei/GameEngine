#include "emitter_panel.h"

#include "editor/widget.h"

namespace my {

void EmitterPanel::UpdateInternal(Scene& p_scene) {
    ParticleEmitterComponent& emitter = p_scene.m_particleEmitter;

    const float column_width = 140.0f;
    DrawVec3Control("Starting velocity", emitter.m_startingVelocity, 0.0f, column_width);
    DrawDragInt("Max particle count", emitter.m_maxParticleCount, 10, 1000, 10000, column_width);
    DrawDragInt("Emit count per second", emitter.m_emittedParticlesPerSecond, 1, 100, 1000, column_width);
    DrawDragFloat("Scaling", emitter.m_particleScale, 0.01f, 0.01f, 10.0f, column_width);
    DrawDragFloat("Life span", emitter.m_particleLifeSpan, 0.1f, 0.1f, 10.0f, column_width);
}

}  // namespace my
