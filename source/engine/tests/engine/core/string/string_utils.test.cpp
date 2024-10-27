#include "core/string/string_utils.h"

namespace my::string_utils {

TEST(StringEqual, null_input) {
    EXPECT_TRUE(StringUtils::StringEqual(nullptr, nullptr));
    EXPECT_TRUE(StringUtils::StringEqual("", nullptr));
    EXPECT_FALSE(StringUtils::StringEqual(nullptr, "abc"));
}

TEST(StringEqual, empty_input) {
    EXPECT_TRUE(StringUtils::StringEqual(nullptr, ""));
    EXPECT_TRUE(StringUtils::StringEqual("", ""));
    EXPECT_FALSE(StringUtils::StringEqual("abc", ""));
}

TEST(StringEqual, succeed) {
    EXPECT_TRUE(StringUtils::StringEqual("abcd", "abcd"));
    EXPECT_TRUE(StringUtils::StringEqual("01234", "01234"));
}

TEST(StringEqual, fail) {
    EXPECT_FALSE(StringUtils::StringEqual("acd", "abcd"));
    EXPECT_FALSE(StringUtils::StringEqual("01", "01234"));
}

TEST(ReplaceFirst, pattern_found) {
    std::string str = "123_123";
    StringUtils::ReplaceFirst(str, "1234", "321");
    EXPECT_EQ(str, "123_123");
}

TEST(ReplaceFirst, pattern_not_found) {
    std::string str = "123_123";
    StringUtils::ReplaceFirst(str, "123", "321");
    EXPECT_EQ(str, "321_123");
}

TEST(Snprintf, format_string) {
    char buffer[64];
    StringUtils::Sprintf(buffer, "(%d %c %s = %u)", 1, '+', "10", 11);

    EXPECT_STREQ(buffer, "(1 + 10 = 11)");
}

TEST(Snprintf, overflow) {
    char buffer[8];
    StringUtils::Sprintf(buffer, "12%s", "345678");

    EXPECT_STREQ(buffer, "1234567");
}

TEST(Snprintf, no_overflow) {
    char buffer[9];
    StringUtils::Sprintf(buffer, "12%s", "345678");

    EXPECT_STREQ(buffer, "12345678");
}

TEST(Strcpy, no_overflow) {
    char buffer[8];
    StringUtils::Strcpy(buffer, "abcdefg");

    EXPECT_STREQ(buffer, "abcdefg");
}

TEST(Strcpy, overflow) {
    char buffer[5];
    StringUtils::Strcpy(buffer, "abcdefg");

    EXPECT_STREQ(buffer, "abcd");
}

}  // namespace my::string_utils
