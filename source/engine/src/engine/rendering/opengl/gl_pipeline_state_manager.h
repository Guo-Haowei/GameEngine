#pragma once
#include "rendering/GLPrerequisites.h"
#include "rendering/pipeline_state_manager.h"

namespace my {

struct GLPipelineState : public PipelineState {
    GLuint program_id;

    ~GLPipelineState() {
        if (program_id) {
            glDeleteProgram(program_id);
        }
    }
};

class GLPipelineStateManager : public PipelineStateManager {
protected:
    std::shared_ptr<PipelineState> create(const PipelineCreateInfo& info) final;
};

}  // namespace my