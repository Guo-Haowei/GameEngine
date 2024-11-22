#include "engine/core/error/error.h"

#include <regex>

#include "engine/core/string/string_builder.h"

namespace my {

using Result = std::expected<void, ErrorRef>;

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

static Result DoStuffImpl() {
    return HBN_ERROR(ERR_BUG, "???");
}

static Result DoStuffWrapper() {
    auto res = DoStuffImpl();
    return HBN_ERROR(res.error());
}

static Result MyFunc() {
    auto res = DoStuffWrapper();
    return HBN_ERROR(res.error());
}

TEST(InternalError, error_stack_trace) {
    auto lambda = []() -> Result {
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
