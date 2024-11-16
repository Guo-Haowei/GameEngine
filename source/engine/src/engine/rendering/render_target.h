#pragma once
#include "rendering/gpu_resource.h"
#include "rendering/render_graph/render_graph_defines.h"

namespace my {

struct RenderTargetDesc {
    RenderTargetResourceName name;
    PixelFormat format;
    AttachmentType type;
    int width;
    int height;
    bool gen_mipmap;
    bool need_uav;

    RenderTargetDesc(RenderTargetResourceName p_name, PixelFormat p_format, AttachmentType p_type, int p_width, int p_height, bool p_gen_mipmap = false, bool p_need_uav = false)
        : name(p_name), format(p_format), type(p_type), width(p_width), height(p_height), gen_mipmap(p_gen_mipmap), need_uav(p_need_uav) {
    }
};

struct RenderTarget {
    RenderTarget(const RenderTargetDesc& p_desc) : desc(p_desc) {}

    std::tuple<int, int> getSize() const { return std::make_tuple(desc.width, desc.height); }
    const RenderTargetDesc desc;
    std::shared_ptr<GpuTexture> texture;
};

}  // namespace my
