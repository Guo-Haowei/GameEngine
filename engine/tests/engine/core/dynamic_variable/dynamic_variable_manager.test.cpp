#include "engine/core/dynamic_variable/dynamic_variable_manager.h"

#include "engine/core/os/os.h"

namespace my {

#include "test_dvars.h"

extern void register_test_dvars();

using Commands = std::vector<std::string>;

TEST(dynamic_variable_parser, invalid_command) {
    register_test_dvars();

    Commands commands = { "+abc" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_FALSE(ok);
    EXPECT_EQ(parser.GetError(), "unknown command '+abc'");
}

TEST(dynamic_variable_parser, invalid_dvar_name) {
    register_test_dvars();

    Commands commands = { "+set", "test_int1" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_FALSE(ok);
    EXPECT_EQ(parser.GetError(), "dvar 'test_int1' not found");
}

TEST(dynamic_variable_parser, unexpected_eof) {
    register_test_dvars();

    Commands commands = { "+set", "test_int" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_FALSE(ok);
    EXPECT_EQ(parser.GetError(), "invalid arguments: +set test_int");
}

TEST(dynamic_variable_parser, set_int) {
    register_test_dvars();

    Commands commands = { "+set", "test_int", "1001" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_INT(test_int), 1001);
}

TEST(dynamic_variable_parser, set_float) {
    register_test_dvars();

    Commands commands = { "+set", "test_float", "1001.1" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_FLOAT(test_float), 1001.1f);
}

TEST(dynamic_variable_parser, set_string) {
    register_test_dvars();

    Commands commands = { "+set", "test_string", "1001.1" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_STRING(test_string), "1001.1");
}

TEST(dynamic_variable_parser, set_vec2) {
    register_test_dvars();

    Commands commands = { "+set", "test_vec2", "6", "7" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_VEC2(test_vec2), NewVector2f(6, 7));
}

TEST(dynamic_variable_parser, set_vec3) {
    register_test_dvars();

    Commands commands = { "+set", "test_vec3", "6", "7", "8" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_VEC3(test_vec3), NewVector3f(6, 7, 8));
}

TEST(dynamic_variable_parser, set_vec4) {
    register_test_dvars();

    Commands commands = { "+set", "test_vec4", "6", "7", "8", "9" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_VEC4(test_vec4), NewVector4f(6, 7, 8, 9));
}

TEST(dynamic_variable_parser, set_ivec2) {
    register_test_dvars();

    Commands commands = { "+set", "test_ivec2", "6", "7" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_IVEC2(test_ivec2), NewVector2i(6, 7));
}

TEST(dynamic_variable_parser, set_ivec3) {
    register_test_dvars();

    Commands commands = { "+set", "test_ivec3", "6", "7", "8" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_IVEC3(test_ivec3), NewVector3i(6, 7, 8));
}

TEST(dynamic_variable_parser, set_ivec4) {
    register_test_dvars();

    Commands commands = { "+set", "test_ivec4", "6", "7", "8", "9" };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_IVEC4(test_ivec4), NewVector4i(6, 7, 8, 9));
}

TEST(dynamic_variable_parser, multiple_set_success) {
    register_test_dvars();

    Commands commands = {
        // clang-format off
        "+set", "test_ivec4", "7", "8", "9", "10",
        "+set", "test_int", "1002",
        // clang-format on
    };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_TRUE(ok);
    EXPECT_EQ(DVAR_GET_IVEC4(test_ivec4), NewVector4i(7, 8, 9, 10));
    EXPECT_EQ(DVAR_GET_INT(test_int), 1002);
}

TEST(dynamic_variable_parser, multiple_set_fail) {
    register_test_dvars();

    Commands commands = {
        // clang-format off
        "+set", "test_ivec4", "7", "8", "9", "10",
        "+set", "test_int", "1002",
        "+set", "test_vec4", "1",
        // clang-format on
    };

    DynamicVariableParser parser{ commands, DynamicVariableParser::SOURCE_NONE };
    bool ok = parser.Parse();
    EXPECT_FALSE(ok);
    EXPECT_EQ(parser.GetError(), "invalid arguments: +set test_vec4 1");
}

}  // namespace my
