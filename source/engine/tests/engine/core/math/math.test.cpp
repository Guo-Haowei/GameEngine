#include "core/math/math.h"

namespace my::math {

TEST(Bit, compile_time) {
    static_assert(Bit(0) == 0);
    static_assert(Bit(1) == 1);
    static_assert(Bit(2) == 2);
    static_assert(Bit(3) == 4);
    static_assert(Bit(31) == 1llu << 30);
    SUCCEED();
}

TEST(Bit, run_time) {
    EXPECT_EQ(Bit(0), 0);
    EXPECT_EQ(Bit(1), 1);
    EXPECT_EQ(Bit(2), 2);
    EXPECT_EQ(Bit(3), 4);
    EXPECT_EQ(Bit(4), 8);
    EXPECT_EQ(Bit(5), 16);
    EXPECT_EQ(Bit(6), 32);
    EXPECT_EQ(Bit(7), 64);
    EXPECT_EQ(Bit(8), 128);
    EXPECT_EQ(Bit(9), 256);
    EXPECT_EQ(Bit(30), 1llu << 29);
    EXPECT_EQ(Bit(31), 1llu << 30);
    EXPECT_EQ(Bit(32), 1llu << 31);
    EXPECT_EQ(Bit(62), 1llu << 61);
    EXPECT_EQ(Bit(63), 1llu << 62);
}

TEST(Align, roundup) {
    EXPECT_EQ(Align(1, 4), 4);
    EXPECT_EQ(Align(2, 4), 4);
    EXPECT_EQ(Align(3, 4), 4);
    EXPECT_EQ(Align(5, 4), 8);
    EXPECT_EQ(Align(6, 4), 8);

    EXPECT_EQ(Align(6, 8), 8);
    EXPECT_EQ(Align(7, 8), 8);
    EXPECT_EQ(Align(9, 8), 16);
    EXPECT_EQ(Align(10, 8), 16);
    EXPECT_EQ(Align(11, 8), 16);
    EXPECT_EQ(Align(12, 8), 16);
    EXPECT_EQ(Align(13, 8), 16);
}

TEST(Align, no_roundup) {
    EXPECT_EQ(Align(8, 4), 8);
    EXPECT_EQ(Align(8, 8), 8);
    EXPECT_EQ(Align(16, 8), 16);
    EXPECT_EQ(Align(128, 128), 128);
}

TEST(Align, uint64) {
    EXPECT_EQ(Align(7llu, 8llu), 8llu);
    EXPECT_EQ(Align(1023llu, 1024llu), 1024llu);
    EXPECT_EQ(Align(1023llu, 256llu), 1024llu);
}

TEST(Align, compile_time) {
    static_assert(Align(10, 16) == 16);
    static_assert(Align(16, 16) == 16);
    static_assert(Align(31, 16) == 32);
}

TEST(log_two, power_of_two) {
    EXPECT_EQ(LogTwo(16), 4);
    EXPECT_EQ(LogTwo(128), 7);
}

TEST(is_power_of_two, success) {
    EXPECT_TRUE(IsPowerOfTwo(16));
    EXPECT_TRUE(IsPowerOfTwo(128));
}

TEST(is_power_of_two, fail) {
    EXPECT_FALSE(IsPowerOfTwo(7));
    EXPECT_FALSE(IsPowerOfTwo(124));
}

TEST(is_power_of_two, compile_time) {
    static_assert(IsPowerOfTwo(16) == true);
    static_assert(IsPowerOfTwo(128) == true);
    static_assert(IsPowerOfTwo(127) == false);
}

TEST(next_power_of_two, roundup) {
    EXPECT_EQ(NextPowerOfTwo(3), 4);
    EXPECT_EQ(NextPowerOfTwo(9), 16);
    EXPECT_EQ(NextPowerOfTwo(10), 16);
    EXPECT_EQ(NextPowerOfTwo(10), 16);
    EXPECT_EQ(NextPowerOfTwo(17), 32);
    EXPECT_EQ(NextPowerOfTwo(39), 64);
}

TEST(next_power_of_two, no_roundup) {
    EXPECT_EQ(NextPowerOfTwo(4), 4);
    EXPECT_EQ(NextPowerOfTwo(8), 8);
    EXPECT_EQ(NextPowerOfTwo(16), 16);
    EXPECT_EQ(NextPowerOfTwo(32), 32);
    EXPECT_EQ(NextPowerOfTwo(64), 64);
}

TEST(next_power_of_two, compile_time) {
    static_assert(NextPowerOfTwo(9) == 16);
    static_assert(NextPowerOfTwo(16) == 16);
    static_assert(NextPowerOfTwo(17) == 32);
}

}  // namespace my::math
