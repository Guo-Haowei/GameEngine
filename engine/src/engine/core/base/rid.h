#pragma once

namespace my {

// Resource's unique identifier
class RID {
public:
    constexpr RID() = default;

    bool is_valid() const { return m_id != 0; }
    bool is_null() const { return m_id == 0; }

    uint64_t get_id() const { return m_id; }

    static RID from_uint64(uint64_t id) {
        RID rid;
        rid.m_id = id;
        return rid;
    }

private:
    uint64_t m_id{ 0 };

    friend class RIDAllocatorBase;
};

}  // namespace my
