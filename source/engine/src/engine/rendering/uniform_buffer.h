#pragma once

namespace my {

// @TODO: rename
class ConstantBufferBase {
public:
    ConstantBufferBase(int p_slot, size_t p_capacity) : m_slot(p_slot), m_capacity(p_capacity) {}

    virtual ~ConstantBufferBase() = default;

    void update(const void* p_data, size_t p_size);

    int GetSlot() const { return m_slot; }
    size_t get_capacity() const { return m_capacity; }

protected:
    const int m_slot;
    const size_t m_capacity;
};

// @TODO: remove this
template<typename T>
struct ConstantBuffer {
    std::shared_ptr<ConstantBufferBase> buffer;
    T cache;

    void update() {
        if (buffer) {
            buffer->update(&cache, sizeof(T));
        }
    }
};

}  // namespace my
