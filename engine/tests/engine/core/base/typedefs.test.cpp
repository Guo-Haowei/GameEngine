#include "engine/core/base/typedefs.h"

namespace my {

TEST(on_scope_exit, test) {
    int counter = 0;
    {
        ON_SCOPE_EXIT([&]() {
            ++counter;
        });
        EXPECT_EQ(counter, 0);
        {
            ON_SCOPE_EXIT([&]() {
                ++counter;
            });
        }
        EXPECT_EQ(counter, 1);
    }
    EXPECT_EQ(counter, 2);
}

enum TestEnum : uint8_t {
    TEST_ENUM_0 = BIT(0),
    TEST_ENUM_1 = BIT(1),
    TEST_ENUM_2 = BIT(2),
    TEST_ENUM_3 = BIT(3),
    TEST_ENUM_4 = BIT(4),
    TEST_ENUM_5 = BIT(5),
    TEST_ENUM_ALL = TEST_ENUM_1 |
                    TEST_ENUM_2 |
                    TEST_ENUM_3 |
                    TEST_ENUM_4 |
                    TEST_ENUM_5,
};
DEFINE_ENUM_BITWISE_OPERATIONS(TestEnum);

TEST(enum_bitwise_operation, test_bitwise_and) {
    constexpr TestEnum flag1 = TEST_ENUM_1 | TEST_ENUM_2 | TEST_ENUM_4;
    constexpr TestEnum flag2 = TEST_ENUM_2 | TEST_ENUM_3 | TEST_ENUM_5;
    EXPECT_EQ(std::to_underlying(flag1 & flag2), 2);
}

TEST(enum_bitwise_operation, test_bitwise_or) {
    EXPECT_EQ(std::to_underlying(TEST_ENUM_1 | TEST_ENUM_2), 3);
}

TEST(enum_bitwise_operation, test_bitwise_and_assign) {
    TestEnum flag1 = TEST_ENUM_1 | TEST_ENUM_2 | TEST_ENUM_4;
    TestEnum flag2 = TEST_ENUM_2 | TEST_ENUM_3 | TEST_ENUM_5;
    flag1 &= flag2;
    EXPECT_EQ(std::to_underlying(flag1), 2);
    EXPECT_EQ(std::to_underlying(flag2), 0b10110);
}

TEST(enum_bitwise_operation, test_bitwise_or_assign) {
    TestEnum flag1 = TEST_ENUM_4;
    TestEnum flag2 = TEST_ENUM_5;
    flag1 |= flag2;
    EXPECT_EQ(std::to_underlying(flag1), 0b11000);
}

TEST(enum_bitwise_operation, test_bitwise_flip) {
    constexpr TestEnum flag1 = TEST_ENUM_1 | TEST_ENUM_2;
    constexpr TestEnum flag2 = TEST_ENUM_ALL & (~flag1);
    constexpr TestEnum flag3 = TEST_ENUM_3 | TEST_ENUM_4 | TEST_ENUM_5;
    static_assert(flag2 == flag3);
    EXPECT_EQ(flag2, flag3);
}

TEST(array_length, test_int_array) {
    int arr[] = { 1, 2, 3, 5 };
    EXPECT_EQ(array_length(arr), 4);
}

TEST(array_length, test_struct_array) {
    struct A {};
    A arr[] = { {}, {} };
    EXPECT_EQ(array_length(arr), 2);
}

}  // namespace my
