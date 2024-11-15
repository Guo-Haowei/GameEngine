#pragma once

#define IN_USE     &&
#define NOT_IN_USE &&!
#define USE_IF(x)  &&((x) ? 1 : 0) &&
#define USING(x)   (1 x 1)

/// Build
#if defined(_DEBUG) || defined(DDEBUG)
#define DEBUG_BUILD   IN_USE
#define RELEASE_BUILD NOT_IN_USE
#else
#define DEBUG_BUILD   NOT_IN_USE
#define RELEASE_BUILD IN_USE
#endif

/// Platform
#if defined(WIN32) || defined(_WIN32)
#define PLATFORM_WINDOWS IN_USE
#define PLATFORM_MACOS   NOT_IN_USE
#elif defined(__APPLE__)
#define PLATFORM_WINDOWS NOT_IN_USE
#define PLATFORM_MACOS   IN_USE
#else
#error Platform not supported!
#endif

/// Compiler
#if USING(DEBUG_BUILD)
#define DISABLE_OPTIMIZATION() static_assert(0, "DISABLE_OPTIMIZATION() should not been used width DEBUG_BUILD")
#define ENABLE_OPTIMIZATION()  static_assert(0, "ENABLE_OPTIMIZATION() should not been used width DEBUG_BUILD")
#else
#define DISABLE_OPTIMIZATION() __pragma(optimize("", off))
#define ENABLE_OPTIMIZATION()  __pragma(optimize("", on))
#endif

/// Warning
#if defined(_MSC_VER)
#define WARNING_PUSH()        __pragma(warning(push))
#define WARNING_POP()         __pragma(warning(pop))
#define WARNING_DISABLE(a, b) __pragma(warning(disable \
                                               : a))
#elif
#define WARNING_PUSH()        __pragma(clang diagnostic push)
#define WARNING_POP()         __pragma(clang diagnostic pop)
#define WARNING_DISABLE(a, b) __pragma(clang diagnostic ignored b)
#else
#error Compiler not supported!
#endif

#if USING(PLATFORM_WINDOWS)
#define GENERATE_TRAP() __debugbreak()
#elif USING(PLATFORM_MACOS)
#define GENERATE_TRAP() __builtin_trap()
#else
#error Platform not supported
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
