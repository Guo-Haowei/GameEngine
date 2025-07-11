#pragma once

namespace my {

struct ComponentFlagBase {
    enum : uint32_t {
        FLAG_NONE = BIT(0),
        FLAG_DIRTY = BIT(31),
    };

    uint32_t flags = FLAG_DIRTY;

    bool IsDirty() const { return flags & FLAG_DIRTY; }
    void SetDirty(bool p_dirty = true) { p_dirty ? flags |= FLAG_DIRTY : flags &= ~FLAG_DIRTY; }
    void OnDeserialized() { flags |= FLAG_DIRTY; }
};

}  // namespace my
