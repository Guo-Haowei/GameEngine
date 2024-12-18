#include <yaml-cpp/yaml.h>

namespace my {

template<typename T>
concept IsArithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept IsIterable = requires(T t) {
    { std::begin(t) } -> std::same_as<typename T::iterator>;
    { std::end(t) } -> std::same_as<typename T::iterator>;
};

struct SerializeYamlContext {
};

struct DeserializeYamlContext {
};

WARNING_PUSH();
WARNING_DISABLE(4100, 4100);

// @TODO: flags
template<typename T>
bool SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context);

template<typename T>
bool DeserializeYaml(const YAML::Emitter& p_node, T& p_object, DeserializeYamlContext& p_context);

template<IsArithmetic T>
bool SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
    p_out << p_object;
    return true;
}

template<IsArithmetic T>
bool DeserializeYaml(const YAML::Node& p_node, T& p_object, DeserializeYamlContext& p_context) {
    // if nullable
    if (!p_node) {
        return true;
    }
    p_object = p_node.as<T>();
    return true;
}

template<IsIterable T>
bool SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
    p_out << YAML::BeginSeq;
    for (const auto& ele : p_object) {
        p_out << ele;
    }
    p_out << YAML::EndSeq;
    return true;
}

template<IsIterable T>
bool DeserializeYaml(const YAML::Node& p_node, T& p_object, DeserializeYamlContext& p_context) {
    // if nullable
    if (!p_node) {
        return true;
    }
    // @TODO: error checking
    const size_t size = p_node.size();
    p_object.resize(size);
    for (size_t i = 0; i < size; ++i) {
        DeserializeYaml(p_node[i], p_object[i], p_context);
    }
    return true;
}

template<>
bool SerializeYaml<std::string>(YAML::Emitter& p_out, const std::string& p_object, SerializeYamlContext& p_context) {
    p_out << p_object;
    return true;
}

template<>
bool DeserializeYaml<std::string>(const YAML::Node& p_node, std::string& p_object, DeserializeYamlContext& p_context) {
    // if nullable
    if (!p_node) {
        return true;
    }

    // @TODO: type check
    p_object = p_node.as<std::string>();
    return true;
}

struct FieldMetaBase {
    FieldMetaBase(const char* p_name,
                  const char* p_type,
                  size_t p_offset) : name(p_name),
                                     type(p_type),
                                     offset(p_offset) {
    }

    const char* name;
    const char* type;
    size_t offset;

    template<typename T>
    const T& GetData(const void* p_object) const {
        char* ptr = (char*)p_object + offset;
        return *reinterpret_cast<T*>(ptr);
    }

    template<typename T>
    T& GetData(void* p_object) {
        char* ptr = (char*)p_object + offset;
        return *reinterpret_cast<T*>(ptr);
    }

    virtual void DumpYaml(YAML::Emitter& p_out, const void* p_object, SerializeYamlContext& p_context) const = 0;
    virtual void UndumpYaml(const YAML::Node& p_node, void* p_object, DeserializeYamlContext& p_context) = 0;
};

template<typename T>
class FieldMeta : public FieldMetaBase {
    using FieldMetaBase::FieldMetaBase;

    void DumpYaml(YAML::Emitter& p_out, const void* p_object, SerializeYamlContext& p_context) const override {
        const T& data = FieldMetaBase::GetData<T>(p_object);
        p_out << YAML::Key << name << YAML::Value;
        SerializeYaml(p_out, data, p_context);
    }

    void UndumpYaml(const YAML::Node& p_node, void* p_object, DeserializeYamlContext& p_context) override {
        T& data = FieldMetaBase::GetData<T>(p_object);
        DeserializeYaml(p_node[name], data, p_context);
    }
};

template<typename T>
class MetaDataTable {
public:
    static const auto& GetFields() {
        return GetFieldsInternal();
    }

    template<typename U>
    static void RegisterField(const char* p_name, const char* p_type, size_t p_offset, const U&) {
        auto field = new FieldMeta<U>(p_name, p_type, p_offset);
        GetFieldsInternal().emplace_back(field);
    }

private:
    static auto& GetFieldsInternal() {
        static std::vector<std::unique_ptr<FieldMetaBase>> s_fields;
        return s_fields;
    }
};

#define REGISTER_FIELD(TYPE, FIELD) \
    MetaDataTable<TYPE>::RegisterField(#FIELD, typeid(FIELD).name(), offsetof(TYPE, FIELD), ((const TYPE*)0)->FIELD)

struct TestClass {

    static void RegisterClass() {
        REGISTER_FIELD(TestClass, v1);
        REGISTER_FIELD(TestClass, v2);
        REGISTER_FIELD(TestClass, v3);
        REGISTER_FIELD(TestClass, v4);
    }

    int v1;
    float v2;
    std::string v3;
    std::vector<int> v4;
};
WARNING_POP();

TEST(aaaaaaaaaaaa, bbbb) {
    TestClass::RegisterClass();
    const auto& meta = MetaDataTable<TestClass>::GetFields();

    YAML::Emitter out;

    const int v1 = 11;
    const float v2 = 12.30f;
    const std::string v3 = "abcdef";
    const std::vector<int> v4 = { 2, 4, 5, 7 };
    {
        out << YAML::BeginMap;

        TestClass a{ v1, v2, v3, v4 };
        SerializeYamlContext context;
        for (const auto& field : meta) {
            field->DumpYaml(out, &a, context);
        }

        out << YAML::EndMap;
    }

    {
        const char* s = out.c_str();
        auto node = YAML::Load(s);
        TestClass a;
        DeserializeYamlContext context;
        for (const auto& field : meta) {
            field->UndumpYaml(node, &a, context);
        }
        EXPECT_EQ(a.v1, v1);
        EXPECT_EQ(a.v2, v2);
        EXPECT_EQ(a.v3, v3);
        EXPECT_EQ(a.v4, v4);
    }
}

}  // namespace my
