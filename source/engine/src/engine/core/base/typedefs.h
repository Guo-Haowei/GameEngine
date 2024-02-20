#pragma once

#define IN_USE     &&
#define NOT_IN_USE &&!
#define USE_IF(x)  &&((x) ? 1 : 0) &&
#define USING(x)   (1 x 1)

#if defined(_DEBUG) || defined(DDEBUG)
#define DEBUG_BUILD   IN_USE
#define RELEASE_BUILD NOT_IN_USE
#else
#define DEBUG_BUILD   NOT_IN_USE
#define RELEASE_BUILD IN_USE
#endif

#if USING(DEBUG_BUILD)
#define DISABLE_OPTIMIZATION() static_assert(0, "DISABLE_OPTIMIZATION() should not been used width DEBUG_BUILD")
#define ENABLE_OPTIMIZATION()  static_assert(0, "ENABLE_OPTIMIZATION() should not been used width DEBUG_BUILD")
#else
#define DISABLE_OPTIMIZATION() __pragma(optimize("", off))
#define ENABLE_OPTIMIZATION()  __pragma(optimize("", on))
#endif

#ifdef _STR
#undef _STR
#endif
#define _STR(x) #x

#define ON_SCOPE_EXIT(FUNC) auto __on_scope_exit_call = ::my::MakeScopeDrop(FUNC)

namespace my {

inline constexpr size_t KB = 1024;
inline constexpr size_t MB = 1024 * KB;
inline constexpr size_t GB = 1024 * MB;

template<typename T, int N>
inline constexpr int array_length(T (&)[N]) {
    return N;
}

template<typename T>
void unused(T &) {}

template<typename FUNC>
class ScopeDrop {
public:
    ScopeDrop(FUNC func) : m_func(func) {}
    ~ScopeDrop() { m_func(); }

private:
    FUNC m_func;
};

template<typename FUNC>
ScopeDrop<FUNC> MakeScopeDrop(FUNC func) {
    return ScopeDrop<FUNC>(func);
}

}  // namespace my
