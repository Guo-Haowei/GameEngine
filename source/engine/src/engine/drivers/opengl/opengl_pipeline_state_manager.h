#pragma once
#include "drivers/opengl/opengl_prerequisites.h"
#include "rendering/pipeline_state_manager.h"

namespace my {

struct OpenGLPipelineState : public PipelineState {
    using PipelineState::PipelineState;

    GLuint program_id;

    ~OpenGLPipelineState() {
        if (program_id) {
            glDeleteProgram(program_id);
        }
    }
};

class OpenGLPipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> create(const PipelineCreateInfo& info) final;
};

}  // namespace my