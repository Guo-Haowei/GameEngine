#pragma once
#include "core/base/intrusive_list.h"
#include "error.h"

namespace my {

#ifdef _MSC_VER
#define GENERATE_TRAP() __debugbreak()
#else
#error "compiler not supported"
#endif

void BreakIfDebug();

using ErrorHandlerFunc = void (*)(void* p_user_data,
                                  std::string_view p_function,
                                  std::string_view p_file,
                                  int p_line,
                                  std::string_view p_error);

struct ErrorHandlerListNode {
    ErrorHandlerFunc errorFunc = nullptr;
    void* userdata = nullptr;
};

using ErrorHandler = IntrusiveList<ErrorHandlerListNode>::Node;

bool AddErrorHandler(ErrorHandler* p_handler);
bool RemoveErrorHandler(const ErrorHandler* p_handler);

void ReportErrorImpl(std::string_view p_function,
                     std::string_view p_file,
                     int p_line,
                     std::string_view p_error,
                     std::string_view p_optional_message);

inline void ReportErrorImpl(std::string_view p_function,
                            std::string_view p_file,
                            int p_line,
                            std::string_view p_error) {
    return ReportErrorImpl(p_function, p_file, p_line, p_error, "");
}

void ReportErrorIndexImpl(std::string_view p_function,
                          std::string_view p_file,
                          int p_line,
                          std::string_view p_prefix,
                          int64_t p_index,
                          int64_t p_bound,
                          std::string_view p_index_string,
                          std::string_view p_bound_string,
                          std::string_view p_detail);

#define ERR_FAIL_V_MSG_INTERNAL(RET, MSG, EXTRA)                                                       \
    do {                                                                                               \
        ::my::ReportErrorImpl(__FUNCTION__, __FILE__, __LINE__, "Method/function failed." EXTRA, MSG); \
        ::my::BreakIfDebug();                                                                          \
        return RET;                                                                                    \
    } while (0)

#define ERR_FAIL_COND_V_MSG_INTERNAL(EXPR, RET, MSG, EXTRA)                                                          \
    if (!!(EXPR)) [[unlikely]] {                                                                                     \
        ::my::ReportErrorImpl(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(EXPR) "\" is true." EXTRA, MSG); \
        ::my::BreakIfDebug();                                                                                        \
        return RET;                                                                                                  \
    } else                                                                                                           \
        ((void)0)

#define ERR_FAIL_INDEX_V_MSG_INTERNAL(INDEX, BOUND, RET, MSG)                                                        \
    if (int64_t index_ = (int64_t)(INDEX), bound_ = (int64_t)(BOUND); index_ < 0 || index_ >= bound_) [[unlikely]] { \
        ::my::ReportErrorIndexImpl(__FUNCTION__, __FILE__, __LINE__, "", index_, bound_, _STR(INDEX), _STR(BOUND),   \
                                   MSG);                                                                             \
        ::my::BreakIfDebug();                                                                                        \
        return RET;                                                                                                  \
    } else                                                                                                           \
        ((void)0)

#define ERR_FAIL()                                   ERR_FAIL_V_MSG_INTERNAL(void(), "", "")
#define ERR_FAIL_MSG(MSG)                            ERR_FAIL_V_MSG_INTERNAL(void(), MSG, "")
#define ERR_FAIL_V(RET)                              ERR_FAIL_V_MSG_INTERNAL(RET, "", " Returning: " _STR(RET))
#define ERR_FAIL_V_MSG(RET, MSG)                     ERR_FAIL_V_MSG_INTERNAL(RET, MSG, " Returning: " _STR(RET))
#define ERR_FAIL_COND(EXPR)                          ERR_FAIL_COND_V_MSG_INTERNAL(EXPR, void(), "", "")
#define ERR_FAIL_COND_MSG(EXPR, MSG)                 ERR_FAIL_COND_V_MSG_INTERNAL(EXPR, void(), MSG, "")
#define ERR_FAIL_COND_V(EXPR, RET)                   ERR_FAIL_COND_V_MSG_INTERNAL(EXPR, RET, "", " Returning: " _STR(RET))
#define ERR_FAIL_COND_V_MSG(EXPR, RET, MSG)          ERR_FAIL_COND_V_MSG_INTERNAL(EXPR, RET, MSG, " Returning: " _STR(RET))
#define ERR_FAIL_INDEX(INDEX, BOUND)                 ERR_FAIL_INDEX_V_MSG_INTERNAL(INDEX, BOUND, void(), "")
#define ERR_FAIL_INDEX_MSG(INDEX, BOUND, MSG)        ERR_FAIL_INDEX_V_MSG_INTERNAL(INDEX, BOUND, void(), MSG)
#define ERR_FAIL_INDEX_V(INDEX, BOUND, RET)          ERR_FAIL_INDEX_V_MSG_INTERNAL(INDEX, BOUND, RET, "")
#define ERR_FAIL_INDEX_V_MSG(INDEX, BOUND, RET, MSG) ERR_FAIL_INDEX_V_MSG_INTERNAL(INDEX, BOUND, RET, MSG)

// @TODO: use same crash handler
// @TODO: flush
#define CRASH_NOW()                                                                              \
    do {                                                                                         \
        my::ReportErrorImpl(__FUNCTION__, __FILE__, __LINE__, "FATAL: Method/function failed."); \
        GENERATE_TRAP();                                                                         \
    } while (0)

#define CRASH_NOW_MSG(MSG)                                                                            \
    do {                                                                                              \
        my::ReportErrorImpl(__FUNCTION__, __FILE__, __LINE__, "FATAL: Method/function failed.", MSG); \
        GENERATE_TRAP();                                                                              \
    } while (0)

#define CRASH_COND(EXPR)                                                                                       \
    if (EXPR) [[unlikely]] {                                                                                   \
        my::ReportErrorImpl(__FUNCTION__, __FILE__, __LINE__, "FATAL: Condition \"" _STR(EXPR) "\" is true."); \
        GENERATE_TRAP();                                                                                       \
    } else                                                                                                     \
        (void)0

#define CRASH_COND_MSG(EXPR, MSG)                                                                                   \
    if (EXPR) [[unlikely]] {                                                                                        \
        my::ReportErrorImpl(__FUNCTION__, __FILE__, __LINE__, "FATAL: Condition \"" _STR(EXPR) "\" is true.", MSG); \
        GENERATE_TRAP();                                                                                            \
    } else                                                                                                          \
        (void)0

#define DEV_ASSERT(EXPR)                                                                                    \
    if (!(EXPR)) [[unlikely]] {                                                                             \
        my::ReportErrorImpl(__FUNCTION__, __FILE__, __LINE__, "FATAL: DEV_ASSERT failed (" _STR(EXPR) ")"); \
        GENERATE_TRAP();                                                                                    \
    } else                                                                                                  \
        ((void)0)

#define DEV_ASSERT_MSG(EXPR, MSG)                                                                                \
    if (!(EXPR)) [[unlikely]] {                                                                                  \
        my::ReportErrorImpl(__FUNCTION__, __FILE__, __LINE__, "FATAL: DEV_ASSERT failed (" _STR(EXPR) ")", MSG); \
        GENERATE_TRAP();                                                                                         \
    } else                                                                                                       \
        ((void)0)

#define DEV_ASSERT_INDEX(INDEX, BOUND)                                                                               \
    if (int64_t index_ = (int64_t)(INDEX), bound_ = (int64_t)(BOUND); index_ < 0 || index_ >= bound_) [[unlikely]] { \
        my::ReportErrorIndexImpl(__FUNCTION__, __FILE__, __LINE__, "FATAL: DEV_ASSERT_INDEX failed ", index_,        \
                                 bound_, _STR(INDEX), _STR(BOUND), "");                                              \
        GENERATE_TRAP();                                                                                             \
    } else                                                                                                           \
        ((void)0)

}  // namespace my