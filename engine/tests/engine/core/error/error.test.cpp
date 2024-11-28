#include "engine/core/error/error.h"

#include <regex>

#include "engine/core/string/string_builder.h"

namespace my {

using Error = InternalError<ErrorCode>;
using ErrorRef = std::shared_ptr<Error>;

template<typename T>
using Result = std::expected<T, ErrorRef>;

TEST(InternalError, constructor_no_string) {
    constexpr size_t LINE_NUMBER = __LINE__;
    auto err = HBN_ERROR(ErrorCode::OK).error();
    EXPECT_EQ(err->line, LINE_NUMBER + 1);
    EXPECT_EQ(err->value, ErrorCode::OK);
    EXPECT_EQ(err->message, "");
}

TEST(InternalError, constructor_with_format) {
    constexpr size_t LINE_NUMBER = __LINE__;
    auto err = HBN_ERROR(ErrorCode::ERR_ALREADY_EXISTS, "({}={}={})", 1, 2, 3).error();
    EXPECT_EQ(err->line, LINE_NUMBER + 1);
    EXPECT_EQ(err->value, ErrorCode::ERR_ALREADY_EXISTS);
    EXPECT_EQ(err->message, "(1=2=3)");
}

TEST(InternalError, constructor_with_long_format) {
    constexpr size_t LINE_NUMBER = __LINE__;
    auto err = HBN_ERROR(ErrorCode::ERR_ALREADY_EXISTS, "({}={}={}={}={}={})", 1, -2, 3, 5.5f, 'c', "def").error();
    EXPECT_EQ(err->line, LINE_NUMBER + 1);
    EXPECT_EQ(err->value, ErrorCode::ERR_ALREADY_EXISTS);
    EXPECT_EQ(err->message, "(1=-2=3=5.5=c=def)");
}

static Result<void> DoStuffImpl() {
    return HBN_ERROR(ErrorCode::ERR_BUG, "???");
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
