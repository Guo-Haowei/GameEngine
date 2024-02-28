#pragma once

// @TODO: rename
// @TODO: range bind
namespace my {

class UniformBufferBase {
public:
    UniformBufferBase(int p_slot, size_t p_capacity) : m_slot(p_slot), m_capacity(p_capacity) {}

    virtual ~UniformBufferBase() = default;

    void update(const void* p_data, size_t p_size);

    void bind(size_t p_size, size_t p_offset) {
        unused(p_size);
        unused(p_offset);
    }

    int get_slot() const { return m_slot; }
    size_t get_capacity() const { return m_capacity; }

protected:
    const int m_slot;
    const size_t m_capacity;
};

// @TODO: enable if
template<typename T>
struct UniformBuffer {
    std::shared_ptr<UniformBufferBase> buffer;
    T cache;

    void update() {
        if (buffer) {
            buffer->update(&cache, sizeof(T));
        }
    }
};

}  // namespace my
