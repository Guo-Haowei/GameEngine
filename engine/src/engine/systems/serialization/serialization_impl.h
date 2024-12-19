#pragma once

namespace my {
class FileAccess;
}

namespace YAML {
class Node;
class Emitter;
}  // namespace YAML

#define DUMP_KEY(a) ::YAML::Key << (a) << ::YAML::Value

namespace my::serialize {

enum FieldFlag : uint32_t {
    NONE = BIT(0),
    NUALLABLE = BIT(1),
    BINARY = BIT(2),
    EMIT_SAME_LINE = BIT(3),
};

DEFINE_ENUM_BITWISE_OPERATIONS(FieldFlag);

struct SerializeYamlContext {
    FieldFlag flags;
    uint32_t version;
    FileAccess* file;
};

struct FieldMetaBase {
    FieldMetaBase(const char* p_name,
                  const char* p_type,
                  size_t p_offset,
                  FieldFlag p_flags) : name(p_name),
                                       type(p_type),
                                       offset(p_offset),
                                       flags(p_flags) {
    }

    virtual ~FieldMetaBase() = default;

    const char* const name;
    const char* const type;
    const size_t offset;
    const FieldFlag flags;

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

    [[nodiscard]] virtual Result<void> DumpYaml(YAML::Emitter& p_out, const void* p_object, SerializeYamlContext& p_context) const = 0;
    [[nodiscard]] virtual Result<void> UndumpYaml(const YAML::Node& p_node, void* p_object, SerializeYamlContext& p_context) = 0;
};

template<typename T>
class FieldMeta : public FieldMetaBase {
    using FieldMetaBase::FieldMetaBase;

    Result<void> DumpYaml(YAML::Emitter& p_out, const void* p_object, SerializeYamlContext& p_context) const override;

    Result<void> UndumpYaml(const YAML::Node& p_node, void* p_object, SerializeYamlContext& p_context) override;
};

template<typename T>
class MetaDataTable {
public:
    static const auto& GetFields() {
        return GetFieldsInternal();
    }

    template<typename U>
    static void RegisterField(const U&, const char* p_name, const char* p_type, size_t p_offset, FieldFlag p_flag) {
        auto& fields = GetFieldsInternal();
#if USING(DEBUG_BUILD)
        for (const auto& field : fields) {
            DEV_ASSERT_MSG(field->name != p_name, std::format("field '{}' already registered", p_name));
        }
#endif
        auto field = new FieldMeta<U>(p_name, p_type, p_offset, p_flag);
        fields.emplace_back(field);
    }

private:
    static auto& GetFieldsInternal() {
        static std::vector<std::unique_ptr<FieldMetaBase>> s_fields;
        return s_fields;
    }
};

#define REGISTER_FIELD(TYPE, NAME, FIELD, FLAGS) \
    ::my::serialize::MetaDataTable<TYPE>::RegisterField(((const TYPE*)0)->FIELD, NAME, typeid(FIELD).name(), offsetof(TYPE, FIELD), FLAGS)

#define REGISTER_FIELD_2(TYPE, FIELD, FLAGS) \
    ::my::serialize::MetaDataTable<TYPE>::RegisterField(((const TYPE*)0)->FIELD, #FIELD, typeid(FIELD).name(), offsetof(TYPE, FIELD), FLAGS)

}  // namespace my::serialize
