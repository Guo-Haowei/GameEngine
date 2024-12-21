#include "engine/core/dynamic_variable/dynamic_variable.h"

#include "../math/vector.test.h"
#include "engine/core/io/archive.h"

namespace my {

#define DEFINE_DVAR
#include "test_dvars.h"

void register_test_dvars() {
    static bool s_registered = false;
    if (!s_registered) {
#define REGISTER_DVAR
#include "test_dvars.h"
#undef REGISTER_DVAR
    }
    s_registered = true;
}

extern void assert_handler(void*, std::string_view, std::string_view, int, std::string_view);

TEST(dynamic_variable, wrong_type) {
    register_test_dvars();

    ErrorHandler handler;
    handler.data.errorFunc = assert_handler;
    AddErrorHandler(&handler);

    EXPECT_EXIT(
        {
            (void)DVAR_GET_INT(test_float);
        },
        testing::ExitedWithCode(99), "m_type == VARIANT_TYPE_INT");

    EXPECT_EXIT(
        {
            (void)DVAR_GET_FLOAT(test_string);
        },
        testing::ExitedWithCode(99), "m_type == VARIANT_TYPE_FLOAT");

    EXPECT_EXIT(
        {
            (void)DVAR_GET_STRING(test_vec2);
        },
        testing::ExitedWithCode(99), "m_type == VARIANT_TYPE_STRING");

    EXPECT_EXIT(
        {
            (void)DVAR_GET_VEC2(test_vec3);
        },
        testing::ExitedWithCode(99), "m_type == VARIANT_TYPE_VEC2");

    EXPECT_EXIT(
        {
            (void)DVAR_GET_VEC3(test_vec4);
        },
        testing::ExitedWithCode(99), "m_type == VARIANT_TYPE_VEC3");

    EXPECT_EXIT(
        {
            (void)DVAR_GET_VEC4(test_ivec2);
        },
        testing::ExitedWithCode(99), "m_type == VARIANT_TYPE_VEC4");

    EXPECT_EXIT(
        {
            (void)DVAR_GET_IVEC2(test_ivec3);
        },
        testing::ExitedWithCode(99), "m_type == VARIANT_TYPE_IVEC2");

    EXPECT_EXIT(
        {
            (void)DVAR_GET_IVEC3(test_ivec4);
        },
        testing::ExitedWithCode(99), "m_type == VARIANT_TYPE_IVEC3");

    RemoveErrorHandler(&handler);
}

TEST(dynamic_variable, int) {
    auto value = DVAR_GET_INT(test_int);
    EXPECT_EQ(value, 100);
    DVAR_SET_INT(test_int, 200);
    value = DVAR_GET_INT(test_int);
    EXPECT_EQ(value, 200);
}

TEST(dynamic_variable, float) {
    auto value = DVAR_GET_FLOAT(test_float);
    EXPECT_EQ(value, 2.3f);
    DVAR_SET_FLOAT(test_float, 1.2f);
    value = DVAR_GET_FLOAT(test_float);
    EXPECT_EQ(value, 1.2f);
}

TEST(dynamic_variable, string) {
    auto value = DVAR_GET_STRING(test_string);
    EXPECT_EQ(value, "abc");
    DVAR_SET_STRING(test_string, std::string_view("bcd"));
    value = DVAR_GET_STRING(test_string);
    EXPECT_EQ(value, "bcd");
}

TEST(dynamic_variable, Vector2f) {
    auto value = DVAR_GET_VEC2(test_vec2);
    EXPECT_EQ(value, Vector2f(1, 2));
    DVAR_SET_VEC2(test_vec2, 7.0f, 8.0f);
    value = DVAR_GET_VEC2(test_vec2);
    EXPECT_EQ(value, Vector2f(7, 8));
}

TEST(dynamic_variable, Vector3f) {
    auto value = DVAR_GET_VEC3(test_vec3);
    EXPECT_EQ(value, Vector3f(1, 2, 3));
    DVAR_SET_VEC3(test_vec3, 7.0f, 8.0f, 9.0f);
    value = DVAR_GET_VEC3(test_vec3);
    EXPECT_EQ(value, Vector3f(7, 8, 9));
}

TEST(dynamic_variable, Vector4f) {
    auto value = DVAR_GET_VEC4(test_vec4);
    EXPECT_EQ(value, Vector4f(1, 2, 3, 4));
    DVAR_SET_VEC4(test_vec4, 7.0f, 8.0f, 9.0f, 10.0f);
    value = DVAR_GET_VEC4(test_vec4);
    EXPECT_EQ(value, Vector4f(7, 8, 9, 10));
}

TEST(dynamic_variable, Vector2i) {
    auto value = DVAR_GET_IVEC2(test_ivec2);
    EXPECT_EQ(value, Vector2i(1, 2));
    DVAR_SET_IVEC2(test_ivec2, 7, 8);
    value = DVAR_GET_IVEC2(test_ivec2);
    EXPECT_EQ(value, Vector2i(7, 8));
}

TEST(dynamic_variable, Vector3i) {
    auto value = DVAR_GET_IVEC3(test_ivec3);
    EXPECT_EQ(value, Vector3i(1, 2, 3));
    DVAR_SET_IVEC3(test_ivec3, 7, 8, 9);
    value = DVAR_GET_IVEC3(test_ivec3);
    EXPECT_EQ(value, Vector3i(7, 8, 9));
}

TEST(dynamic_variable, Vector4i) {
    auto value = DVAR_GET_IVEC4(test_ivec4);
    EXPECT_EQ(value, Vector4i(1, 2, 3, 4));
    DVAR_SET_IVEC4(test_ivec4, 7, 8, 9, 10);
    value = DVAR_GET_IVEC4(test_ivec4);
    EXPECT_EQ(value, Vector4i(7, 8, 9, 10));
}

}  // namespace my
