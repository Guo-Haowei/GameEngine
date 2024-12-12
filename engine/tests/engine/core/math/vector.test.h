#pragma once
#include "vector.h"

#define CHECK_VEC2(VEC, a, b)  \
    {                          \
        EXPECT_EQ((VEC).x, a); \
        EXPECT_EQ((VEC).y, b); \
    }
#define CHECK_VEC3(VEC, a, b, c) \
    {                            \
        EXPECT_EQ((VEC).x, a);   \
        EXPECT_EQ((VEC).y, b);   \
        EXPECT_EQ((VEC).z, c);   \
    }
#define CHECK_VEC4(VEC, a, b, c, d) \
    {                               \
        EXPECT_EQ((VEC).x, a);      \
        EXPECT_EQ((VEC).y, b);      \
        EXPECT_EQ((VEC).z, c);      \
        EXPECT_EQ((VEC).w, d);      \
    }
