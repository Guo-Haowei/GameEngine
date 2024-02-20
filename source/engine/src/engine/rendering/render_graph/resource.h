#pragma once
#include "rendering/pixel_format.h"

namespace my::rg {

enum ResourceType {
    RT_COLOR_ATTACHMENT,
    RT_DEPTH_ATTACHMENT,
    RT_SHADOW_MAP,
    RT_SHADOW_CUBE_MAP,
};

struct ResourceDesc {
    std::string name;
    PixelFormat format;
    ResourceType type;
    int width;
    int height;

    ResourceDesc(const std::string& p_name, PixelFormat p_format, ResourceType p_type, int p_width, int p_height)
        : name(p_name), format(p_format), type(p_type), width(p_width), height(p_height) {
    }
};

class Resource {
public:
    Resource(const ResourceDesc& p_desc) : m_desc(p_desc) {}

    const ResourceDesc& get_desc() const { return m_desc; }
    uint32_t get_handle() const { return m_handle; }
    uint64_t get_resident_handle() const { return m_resident_handle; }

private:
    const ResourceDesc m_desc;
    uint32_t m_handle = 0;
    uint64_t m_resident_handle = 0;

    friend class RenderGraph;
};

}  // namespace my::rg
