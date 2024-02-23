#pragma once
#include "rendering/render_graph/subpass.h"

namespace my {

class OpenGLSubpass : public Subpass {
public:
    void set_render_target(int p_index) const override;

private:
    uint32_t m_handle;

    friend class OpenGLGraphicsManager;
};

}  // namespace my
