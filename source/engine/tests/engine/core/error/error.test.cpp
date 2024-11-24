#include "engine/core/error/error.h"

#include <regex>

#include "engine/core/string/string_builder.h"

namespace my {

using Error = InternalError<ErrorCode>;
using ErrorRef = std::shared_ptr<Error>;

template<typename T>
using Result = std::expected<T, ErrorRef>;

template<typename T>
[[nodiscard]] static inline constexpr auto _create_error_arg_1(std::string_view p_file,
                                                               std::string_view p_function,
                                                               int p_line,
                                                               T p_value) {
    return std::unexpected(std::shared_ptr<InternalError<T>>(new InternalError<T>(p_file, p_function, p_line, p_value)));
};

template<typename T>
[[nodiscard]] static inline constexpr auto _create_error_arg_1(std::string_view p_file,
                                                               std::string_view p_function,
                                                               int p_line,
                                                               std::shared_ptr<InternalError<T>>& p_error) {
    return std::unexpected(std::shared_ptr<InternalError<T>>(new InternalError<T>(p_file, p_function, p_line, p_error)));
};

template<typename T, typename... Args>
[[nodiscard]] static inline constexpr auto _create_error_arg_2_plus(std::string_view p_file,
                                                                    std::string_view p_function,
                                                                    int p_line,
                                                                    T p_value,
                                                                    std::format_string<Args...> p_format,
                                                                    Args&&... p_args) {
    return std::unexpected(std::shared_ptr<InternalError<T>>(new InternalError<T>(p_file, p_function, p_line, p_value, p_format, std::forward<Args>(p_args)...)));
};

// @TODO: move to a more general place
#define HBN_MACRO_EXPAND(x)                                        x
#define HBN_GET_MACRO_2(_1, _2, FUNC, ...)                         FUNC
#define HBN_GET_MACRO_3(_1, _2, _3, FUNC, ...)                     FUNC
#define HBN_GET_MACRO_4(_1, _2, _3, _4, FUNC, ...)                 FUNC
#define HBN_GET_MACRO_5(_1, _2, _3, _4, _5, FUNC, ...)             FUNC
#define HBN_GET_MACRO_6(_1, _2, _3, _4, _5, _6, FUNC, ...)         FUNC
#define HBN_GET_MACRO_7(_1, _2, _3, _4, _5, _6, _7, FUNC, ...)     FUNC
#define HBN_GET_MACRO_8(_1, _2, _3, _4, _5, _6, _7, _8, FUNC, ...) FUNC

#define HBN_ERROR_1(_1)                             _create_error_arg_1<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1)
#define HBN_ERROR_2(_1, _2)                         _create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2)
#define HBN_ERROR_3(_1, _2, _3)                     _create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3)
#define HBN_ERROR_4(_1, _2, _3, _4)                 _create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4)
#define HBN_ERROR_5(_1, _2, _3, _4, _5)             _create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4, _5)
#define HBN_ERROR_6(_1, _2, _3, _4, _5, _6)         _create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4, _5, _6)
#define HBN_ERROR_7(_1, _2, _3, _4, _5, _6, _7)     _create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4, _5, _6, _7)
#define HBN_ERROR_8(_1, _2, _3, _4, _5, _6, _7, _8) _create_error_arg_2_plus<ErrorCode>(__FILE__, __FUNCTION__, __LINE__, _1, _2, _3, _4, _5, _6, _7, _8)

#undef HBN_ERROR
#define HBN_ERROR(...) HBN_MACRO_EXPAND(HBN_GET_MACRO_8(__VA_ARGS__, HBN_ERROR_8, HBN_ERROR_7, HBN_ERROR_6, HBN_ERROR_5, HBN_ERROR_4, HBN_ERROR_3, HBN_ERROR_2, HBN_ERROR_1)(__VA_ARGS__))

TEST(InternalError, constructor_no_string) {
    constexpr size_t LINE_NUMBER = __LINE__;
    auto err = HBN_ERROR(OK).error();
    EXPECT_EQ(err->line, LINE_NUMBER + 1);
    EXPECT_EQ(err->value, OK);
    EXPECT_EQ(err->message, "");
}

TEST(InternalError, constructor_with_format) {
    constexpr size_t LINE_NUMBER = __LINE__;
    auto err = HBN_ERROR(ERR_ALREADY_EXISTS, "({}={}={})", 1, 2, 3).error();
    EXPECT_EQ(err->line, LINE_NUMBER + 1);
    EXPECT_EQ(err->value, ERR_ALREADY_EXISTS);
    EXPECT_EQ(err->message, "(1=2=3)");
}

TEST(InternalError, constructor_with_long_format) {
    constexpr size_t LINE_NUMBER = __LINE__;
    auto err = HBN_ERROR(ERR_ALREADY_EXISTS, "({}={}={}={}={}={})", 1, -2, 3, 5.5f, 'c', "def").error();
    EXPECT_EQ(err->line, LINE_NUMBER + 1);
    EXPECT_EQ(err->value, ERR_ALREADY_EXISTS);
    EXPECT_EQ(err->message, "(1=-2=3=5.5=c=def)");
}

static Result<void> DoStuffImpl() {
    return HBN_ERROR(ERR_BUG, "???");
}

static Result<void> DoStuffWrapper() {
    auto res = DoStuffImpl();
    return HBN_ERROR(res.error());
}

static Result<void> MyFunc() {
    auto res = DoStuffWrapper();
    return HBN_ERROR(res.error());
}

TEST(InternalError, error_stack_trace) {
    auto lambda = []() -> Result<void> {
        auto res = MyFunc();
        if (!res) {
            return HBN_ERROR(res.error());
        }
        return res;
    };
    auto res = lambda();
    EXPECT_TRUE(!res);

    StringStreamBuilder builder;
    builder << res.error();
    const auto s = builder.ToString();

    std::regex word_regex("(DoStuffWrapper|DoStuffImpl|MyFunc)");
    auto words_begin = std::sregex_iterator(s.begin(), s.end(), word_regex);
    auto words_end = std::sregex_iterator();

    const char* functions[] = {
        "DoStuffImpl",
        "DoStuffWrapper",
        "MyFunc",
    };

    int counter = 0;
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string match_str = match.str();
        EXPECT_EQ(match_str, functions[counter++]);
    }
}

}  // namespace my
