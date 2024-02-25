#pragma once
#include "rendering/render_target.h"

namespace my {

struct Subpass;

using SubPassFunc = void (*)(const Subpass*);

struct SubpassDesc {
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;
    SubPassFunc func = nullptr;
};

struct Subpass {
    SubPassFunc func;
    std::vector<std::shared_ptr<RenderTarget>> color_attachments;
    std::shared_ptr<RenderTarget> depth_attachment;

    virtual void set_render_target(int p_index = 0, int p_miplevel = 0) const = 0;
};

}  // namespace my
