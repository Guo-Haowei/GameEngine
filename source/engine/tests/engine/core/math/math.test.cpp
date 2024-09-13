#include "core/math/math.h"

namespace my::math {

TEST(bit, compile_time) {
    static_assert(bit(0) == 0);
    static_assert(bit(1) == 1);
    static_assert(bit(2) == 2);
    static_assert(bit(3) == 4);
    static_assert(bit(31) == 1llu << 30);
    SUCCEED();
}

TEST(bit, run_time) {
    EXPECT_EQ(bit(0), 0);
    EXPECT_EQ(bit(1), 1);
    EXPECT_EQ(bit(2), 2);
    EXPECT_EQ(bit(3), 4);
    EXPECT_EQ(bit(4), 8);
    EXPECT_EQ(bit(5), 16);
    EXPECT_EQ(bit(6), 32);
    EXPECT_EQ(bit(7), 64);
    EXPECT_EQ(bit(8), 128);
    EXPECT_EQ(bit(9), 256);
    EXPECT_EQ(bit(30), 1llu << 29);
    EXPECT_EQ(bit(31), 1llu << 30);
    EXPECT_EQ(bit(32), 1llu << 31);
    EXPECT_EQ(bit(62), 1llu << 61);
    EXPECT_EQ(bit(63), 1llu << 62);
}

TEST(align, roundup) {
    EXPECT_EQ(align(1, 4), 4);
    EXPECT_EQ(align(2, 4), 4);
    EXPECT_EQ(align(3, 4), 4);
    EXPECT_EQ(align(5, 4), 8);
    EXPECT_EQ(align(6, 4), 8);

    EXPECT_EQ(align(6, 8), 8);
    EXPECT_EQ(align(7, 8), 8);
    EXPECT_EQ(align(9, 8), 16);
    EXPECT_EQ(align(10, 8), 16);
    EXPECT_EQ(align(11, 8), 16);
    EXPECT_EQ(align(12, 8), 16);
    EXPECT_EQ(align(13, 8), 16);
}

TEST(align, no_roundup) {
    EXPECT_EQ(align(8, 4), 8);
    EXPECT_EQ(align(8, 8), 8);
    EXPECT_EQ(align(16, 8), 16);
    EXPECT_EQ(align(128, 128), 128);
}

TEST(align, uint64) {
    EXPECT_EQ(align(7llu, 8llu), 8llu);
    EXPECT_EQ(align(1023llu, 1024llu), 1024llu);
    EXPECT_EQ(align(1023llu, 256llu), 1024llu);
}

TEST(align, compile_time) {
    static_assert(align(10, 16) == 16);
    static_assert(align(16, 16) == 16);
    static_assert(align(31, 16) == 32);
}

TEST(log_two, power_of_two) {
    EXPECT_EQ(logTwo(16), 4);
    EXPECT_EQ(logTwo(128), 7);
}

TEST(is_power_of_two, success) {
    EXPECT_TRUE(isPowerOfTwo(16));
    EXPECT_TRUE(isPowerOfTwo(128));
}

TEST(is_power_of_two, fail) {
    EXPECT_FALSE(isPowerOfTwo(7));
    EXPECT_FALSE(isPowerOfTwo(124));
}

TEST(is_power_of_two, compile_time) {
    static_assert(isPowerOfTwo(16) == true);
    static_assert(isPowerOfTwo(128) == true);
    static_assert(isPowerOfTwo(127) == false);
}

TEST(next_power_of_two, roundup) {
    EXPECT_EQ(nextPowerOfTwo(3), 4);
    EXPECT_EQ(nextPowerOfTwo(9), 16);
    EXPECT_EQ(nextPowerOfTwo(10), 16);
    EXPECT_EQ(nextPowerOfTwo(10), 16);
    EXPECT_EQ(nextPowerOfTwo(17), 32);
    EXPECT_EQ(nextPowerOfTwo(39), 64);
}

TEST(next_power_of_two, no_roundup) {
    EXPECT_EQ(nextPowerOfTwo(4), 4);
    EXPECT_EQ(nextPowerOfTwo(8), 8);
    EXPECT_EQ(nextPowerOfTwo(16), 16);
    EXPECT_EQ(nextPowerOfTwo(32), 32);
    EXPECT_EQ(nextPowerOfTwo(64), 64);
}

TEST(next_power_of_two, compile_time) {
    static_assert(nextPowerOfTwo(9) == 16);
    static_assert(nextPowerOfTwo(16) == 16);
    static_assert(nextPowerOfTwo(17) == 32);
}

}  // namespace my::math
