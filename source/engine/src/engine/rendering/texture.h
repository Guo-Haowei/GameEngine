#pragma once

namespace my {

struct Texture {
    virtual ~Texture() = default;

    virtual uint32_t get_handle() const = 0;
    virtual uint64_t get_resident_handle() const = 0;
    virtual uint64_t get_imgui_handle() const = 0;
};

}  // namespace my
