#pragma once
#include "rendering/render_graph/sub_pass.h"

namespace my {

class GLSubpass : public Subpass {
public:
    void set_render_target() const override;
    void set_render_target_level(int p_level) const override;

private:
    uint32_t m_handle;

    friend class GLGraphicsManager;
};

}  // namespace my
