#pragma once
#include "rendering/pixel_format.h"
#include "rendering/texture.h"

namespace my {

enum RenderTargetType {
    RT_COLOR_ATTACHMENT_2D,
    RT_COLOR_ATTACHMENT_CUBE_MAP,
    RT_DEPTH_ATTACHMENT_2D,
    RT_SHADOW_2D,
    RT_SHADOW_CUBE_MAP,
};

struct RenderTargetDesc {
    std::string name;
    PixelFormat format;
    RenderTargetType type;
    int width;
    int height;
    bool gen_mipmap;

    RenderTargetDesc(const std::string& p_name, PixelFormat p_format, RenderTargetType p_type, int p_width, int p_height, bool p_gen_mipmap = false)
        : name(p_name), format(p_format), type(p_type), width(p_width), height(p_height), gen_mipmap(p_gen_mipmap) {
    }
};

class RenderTarget {
public:
    RenderTarget(const RenderTargetDesc& p_desc) : m_desc(p_desc) {}

    const RenderTargetDesc& get_desc() const { return m_desc; }
    std::tuple<int, int> get_size() const { return std::make_tuple(m_desc.width, m_desc.height); }
    uint32_t get_handle() const { return m_handle; }
    uint64_t get_resident_handle() const { return m_resident_handle; }

private:
    const RenderTargetDesc m_desc;

    // @TODO: refactor this
    uint32_t m_handle = 0;
    uint64_t m_resident_handle = 0;

    friend class OpenGLGraphicsManager;
};

}  // namespace my
