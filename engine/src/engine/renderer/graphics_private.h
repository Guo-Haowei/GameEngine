#ifdef GRAPHICS_PRIVATE_H_INCLUDED
#error DO NOT INCLUDE THIS IN A HEADER FILE
#endif
#define GRAPHICS_PRIVATE_H_INCLUDED

namespace my {

template<typename T>
static inline size_t VectorSizeInByte(const std::vector<T>& vec) {
    static_assert(std::is_trivially_copyable<T>());
    return vec.size() * sizeof(T);
}

}  // namespace my
