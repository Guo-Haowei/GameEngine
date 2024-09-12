#include "engine/core/error/error.h"

namespace my {

TEST(error, constructor_no_string) {
    constexpr size_t LINE_NUMBER = __LINE__;
    auto err = VCT_ERROR(100).error();
    EXPECT_EQ(err.line, LINE_NUMBER + 1);
    EXPECT_EQ(err.getValue(), 100);
    EXPECT_EQ(err.getMessage(), "");
}

TEST(error, constructor_with_format) {
    constexpr size_t LINE_NUMBER = __LINE__;
    auto err = VCT_ERROR(10.0f, "({}={}={})", 1, 2, 3).error();
    EXPECT_EQ(err.line, LINE_NUMBER + 1);
    EXPECT_EQ(err.getValue(), 10.0f);
    EXPECT_EQ(err.getMessage(), "(1=2=3)");
}

}  // namespace my
