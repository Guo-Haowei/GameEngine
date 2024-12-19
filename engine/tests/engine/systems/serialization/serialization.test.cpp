#include "engine/systems/serialization/serialization.h"

#include "engine/scene/light_component.h"
#include "engine/scene/scene_component.h"

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

TEST(RegisterClass, nested_struct) {
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

TEST(RegisterClass, name_component) {
    NameComponent::RegisterClass();

    constexpr const char* MY_NAME = "1_NAME_1";
    YAML::Emitter out;

    {
        NameComponent component;
        component.SetName(MY_NAME);
        SerializeYamlContext context;
        SerializeYaml(out, component, context);
    }
    {
        const char* s = out.c_str();
        auto node = YAML::Load(s);
        EXPECT_EQ(node["name"].as<std::string>(), MY_NAME);

        NameComponent component;
        SerializeYamlContext context;
        DeserializeYaml(node, component, context);
        EXPECT_EQ(component.GetName(), MY_NAME);
    }
}

TEST(RegisterClass, light_component) {
    LightComponent::RegisterClass();
    LightComponent::Attenuation::RegisterClass();

    YAML::Emitter out;
    {
        LightComponent component;
        component.m_atten.constant = 2.0f;
        component.m_atten.linear = 1.0f;
        component.m_atten.quadratic = 3.0f;
        component.m_shadowRegion = AABB(Vector3f(-1, -2, -3), Vector3f(1, 2, 3));
        SerializeYamlContext context;
        SerializeYaml(out, component, context);
    }
    {
        const char* s = out.c_str();
        auto node = YAML::Load(s);

        LightComponent component;
        SerializeYamlContext context;
        DeserializeYaml(node, component, context);
        EXPECT_EQ(component.m_atten.constant, 2.0f);
        EXPECT_EQ(component.m_atten.linear, 1.0f);
        EXPECT_EQ(component.m_atten.quadratic, 3.0f);
        const auto& min = component.m_shadowRegion.GetMin();
        const auto& max = component.m_shadowRegion.GetMax();
        EXPECT_EQ(min.x, -1);
        EXPECT_EQ(min.y, -2);
        EXPECT_EQ(min.z, -3);
        EXPECT_EQ(max.x, 1);
        EXPECT_EQ(max.y, 2);
        EXPECT_EQ(max.z, 3);
    }
}

}  // namespace my::serialize
