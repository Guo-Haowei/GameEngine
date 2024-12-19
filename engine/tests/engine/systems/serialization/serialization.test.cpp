#include "engine/systems/serialization/serialization.inl"

#include <yaml-cpp/yaml.h>

namespace my {

struct OuterClass {
    struct InnerClass {
        std::string name;

        static void RegisterClass() {
            using serialize::FieldFlag;
            REGISTER_FIELD(InnerClass, "name", name, FieldFlag::NONE);
        }
    };

    int v1;
    float v2;
    std::string v3;
    std::vector<int> v4;
    InnerClass v5;

    static void RegisterClass() {
        using serialize::FieldFlag;

        REGISTER_FIELD(OuterClass, "int", v1, FieldFlag::NONE);
        REGISTER_FIELD(OuterClass, "float", v2, FieldFlag::NONE);
        REGISTER_FIELD(OuterClass, "string", v3, FieldFlag::NONE);
        REGISTER_FIELD(OuterClass, "vector<int>", v4, FieldFlag::NONE);
        REGISTER_FIELD(OuterClass, "InnerClass", v5, FieldFlag::NONE);
    }
};

}  // namespace my

namespace my::serialize {

TEST(aaaaaaaaaaaa, bbbb) {
    OuterClass::InnerClass::RegisterClass();
    OuterClass::RegisterClass();

    YAML::Emitter out;
    const int v1 = 11;
    const float v2 = 12.30f;
    const std::string v3 = "abcdef";
    const std::vector<int> v4 = { 2, 4, 5, 7 };
    const std::string name = "<my name>";
    {
        OuterClass a{ v1, v2, v3, v4 };
        a.v5.name = name;
        SerializeYamlContext context;
        SerializeYaml(out, a, context);
    }

    {
        const char* s = out.c_str();
        auto node = YAML::Load(s);
        OuterClass a;
        SerializeYamlContext context;
        DeserializeYaml(node, a, context);

        EXPECT_EQ(a.v1, v1);
        EXPECT_EQ(a.v2, v2);
        EXPECT_EQ(a.v3, v3);
        EXPECT_EQ(a.v4, v4);
        EXPECT_EQ(a.v5.name, name);
    }
}

}  // namespace my::serialize
