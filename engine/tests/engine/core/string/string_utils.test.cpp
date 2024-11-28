#include "engine/core/string/string_utils.h"

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

TEST(StringSplitter, null_string) {
    const char* source = nullptr;
    StringSplitter sp(source);
    int counter = 0;
    while (sp.CanAdvance()) {
        [[maybe_unused]] auto sv = sp.Advance('+');
        ++counter;
    }
    EXPECT_EQ(counter, 0);
}

TEST(StringSplitter, empty_string) {
    const char* source = "";
    StringSplitter sp(source);
    int counter = 0;
    while (sp.CanAdvance()) {
        [[maybe_unused]] auto sv = sp.Advance('+');
        ++counter;
    }
    EXPECT_EQ(counter, 0);
}

TEST(StringSplitter, no_pattern) {
    const char* source = "abc_def";
    StringSplitter sp(source);
    while (sp.CanAdvance()) {
        [[maybe_unused]] auto sv = sp.Advance('+');
        EXPECT_EQ(sv, "abc_def");
    }
}

TEST(StringSplitter, one_pattern) {
    const char* source = "abc def";
    StringSplitter sp(source);
    const char* expectations[] = { "abc", "def" };
    for (int i = 0; sp.CanAdvance(); ++i) {
        [[maybe_unused]] auto sv = sp.Advance(' ');
        EXPECT_EQ(sv, expectations[i]);
    }
}

TEST(StringSplitter, empty_string_after_pattern) {
    const char* source = "abcdef/";
    StringSplitter sp(source);
    const char* expectations[] = { "abcdef" };
    for (int i = 0; sp.CanAdvance(); ++i) {
        [[maybe_unused]] auto sv = sp.Advance('/');
        EXPECT_EQ(sv, expectations[i]);
    }
}

TEST(StringSplitter, multi_pattern) {
    const char* source = "D:\\random\\path\\to\\my\\file\\";
    StringSplitter sp(source);
    const char* expectations[] = { "D:", "random", "path", "to", "my", "file" };
    for (int i = 0; sp.CanAdvance(); ++i) {
        [[maybe_unused]] auto sv = sp.Advance('\\');
        EXPECT_EQ(sv, expectations[i]);
        // printf("{%s}\n", expectations[i]);
    }
}

TEST(StringUtils, is_digit) {
    EXPECT_TRUE(StringUtils::IsDigit('0'));
    EXPECT_TRUE(StringUtils::IsDigit('1'));
    EXPECT_TRUE(StringUtils::IsDigit('2'));
    EXPECT_TRUE(StringUtils::IsDigit('5'));
    EXPECT_TRUE(StringUtils::IsDigit('8'));
    EXPECT_TRUE(StringUtils::IsDigit('9'));
    EXPECT_FALSE(StringUtils::IsDigit('9' + 1));
    EXPECT_FALSE(StringUtils::IsDigit('A'));
    EXPECT_FALSE(StringUtils::IsDigit('a'));
    EXPECT_FALSE(StringUtils::IsDigit('+'));
    EXPECT_FALSE(StringUtils::IsDigit('-'));
}

TEST(StringUtils, is_hex) {
    EXPECT_TRUE(StringUtils::IsHex('0'));
    EXPECT_TRUE(StringUtils::IsHex('1'));
    EXPECT_TRUE(StringUtils::IsHex('2'));
    EXPECT_TRUE(StringUtils::IsHex('5'));
    EXPECT_TRUE(StringUtils::IsHex('8'));
    EXPECT_TRUE(StringUtils::IsHex('9'));
    EXPECT_TRUE(StringUtils::IsHex('A'));
    EXPECT_TRUE(StringUtils::IsHex('a'));
    EXPECT_TRUE(StringUtils::IsHex('B'));
    EXPECT_TRUE(StringUtils::IsHex('b'));
    EXPECT_TRUE(StringUtils::IsHex('E'));
    EXPECT_TRUE(StringUtils::IsHex('e'));
    EXPECT_TRUE(StringUtils::IsHex('F'));
    EXPECT_TRUE(StringUtils::IsHex('f'));
    EXPECT_FALSE(StringUtils::IsHex('G'));
    EXPECT_FALSE(StringUtils::IsHex('g'));
    EXPECT_FALSE(StringUtils::IsHex('9' + 1));
    EXPECT_FALSE(StringUtils::IsHex('+'));
    EXPECT_FALSE(StringUtils::IsHex('-'));
}

TEST(StringUtils, hex_to_int) {
    EXPECT_EQ(StringUtils::HexToInt('0'), 0);
    EXPECT_EQ(StringUtils::HexToInt('2'), 2);
    EXPECT_EQ(StringUtils::HexToInt('5'), 5);
    EXPECT_EQ(StringUtils::HexToInt('8'), 8);
    EXPECT_EQ(StringUtils::HexToInt('9'), 9);
    EXPECT_EQ(StringUtils::HexToInt('f'), 15);
    EXPECT_EQ(StringUtils::HexToInt('F'), 15);
    EXPECT_EQ(StringUtils::HexToInt('a'), 10);
    EXPECT_EQ(StringUtils::HexToInt('A'), 10);
    EXPECT_EQ(StringUtils::HexToInt('c'), 12);
    EXPECT_EQ(StringUtils::HexToInt('E'), 14);
    EXPECT_EQ(StringUtils::HexToInt('*'), -1);
    EXPECT_EQ(StringUtils::HexToInt('-'), -1);
}

}  // namespace my::string_utils
