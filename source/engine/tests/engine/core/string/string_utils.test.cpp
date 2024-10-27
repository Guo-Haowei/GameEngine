#include "core/string/string_utils.h"

namespace my::string_utils {

TEST(stringEqual, null_input) {
    EXPECT_TRUE(StringUtils::stringEqual(nullptr, nullptr));
    EXPECT_TRUE(StringUtils::stringEqual("", nullptr));
    EXPECT_FALSE(StringUtils::stringEqual(nullptr, "abc"));
}

TEST(stringEqual, empty_input) {
    EXPECT_TRUE(StringUtils::stringEqual(nullptr, ""));
    EXPECT_TRUE(StringUtils::stringEqual("", ""));
    EXPECT_FALSE(StringUtils::stringEqual("abc", ""));
}

TEST(stringEqual, succeed) {
    EXPECT_TRUE(StringUtils::stringEqual("abcd", "abcd"));
    EXPECT_TRUE(StringUtils::stringEqual("01234", "01234"));
}

TEST(stringEqual, fail) {
    EXPECT_FALSE(StringUtils::stringEqual("acd", "abcd"));
    EXPECT_FALSE(StringUtils::stringEqual("01", "01234"));
}

TEST(replaceFirst, pattern_found) {
    std::string str = "123_123";
    StringUtils::replaceFirst(str, "1234", "321");
    EXPECT_EQ(str, "123_123");
}

TEST(replaceFirst, pattern_not_found) {
    std::string str = "123_123";
    StringUtils::replaceFirst(str, "123", "321");
    EXPECT_EQ(str, "321_123");
}

}  // namespace my::string_utils
