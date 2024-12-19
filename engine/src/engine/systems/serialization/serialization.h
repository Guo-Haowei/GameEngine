#pragma once
#include <yaml-cpp/yaml.h>

#include "engine/core/math/aabb.h"
#include "engine/core/math/angle.h"
#include "engine/systems/ecs/entity.h"
#include "engine/systems/serialization/serialization.h"

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

template<typename T>
concept IsArithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept IsVector = std::same_as<std::remove_cv_t<T>, std::vector<typename T::value_type>>;

template<typename T>
concept HasMetaTable = requires(T) {
    { T::RegisterClass() } -> std::same_as<void>;
};

template<IsArithmetic T>
Result<void> SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext&) {
    p_out << p_object;
    return Result<void>();
}

template<IsArithmetic T>
Result<void> DeserializeYaml(const YAML::Node& p_node, T& p_object, SerializeYamlContext&) {
    if (!p_node) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Not defined");
    }

    if (!p_node.IsScalar()) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Expect scalar");
    }

    p_object = p_node.as<T>();
    return Result<void>();
}

Result<void> SerializeYaml(YAML::Emitter& p_out, const ecs::Entity& p_object, SerializeYamlContext&) {
    p_out << p_object.GetId();
    return Result<void>();
}

Result<void> DeserializeYaml(const YAML::Node& p_node, ecs::Entity& p_object, SerializeYamlContext&) {
    if (!p_node) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Not defined");
    }

    if (!p_node.IsScalar()) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Expect scalar");
    }

    p_object = ecs::Entity(p_node.as<uint32_t>());
    return Result<void>();
}

Result<void> SerializeYaml(YAML::Emitter& p_out, const Degree& p_object, SerializeYamlContext&) {
    p_out << p_object.GetDegree();
    return Result<void>();
}

Result<void> DeserializeYaml(const YAML::Node& p_node, Degree& p_object, SerializeYamlContext&) {
    if (!p_node) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Not defined");
    }

    if (!p_node.IsScalar()) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Expect scalar");
    }

    p_object = Degree(p_node.as<float>());
    return Result<void>();
}

template<typename T, int N>
Result<void> SerializeYaml(YAML::Emitter& p_out, const glm::vec<N, T, glm::defaultp>& p_object, SerializeYamlContext&) {
    p_out.SetSeqFormat(YAML::Flow);
    p_out << YAML::BeginSeq;
    p_out << p_object.x;
    p_out << p_object.y;
    if constexpr (N > 2) {
        p_out << p_object.z;
    }
    if constexpr (N > 3) {
        p_out << p_object.w;
    }
    p_out << YAML::EndSeq;
    return Result<void>();
}

template<typename T, int N>
Result<void> DeserializeYaml(const YAML::Node& p_node, glm::vec<N, T, glm::defaultp>& p_object, SerializeYamlContext&) {
    if (!p_node) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Not defined");
    }

    if (!p_node.IsSequence() || p_node.size() != N) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Expect sequence of {}", N);
    }

    p_object.x = p_node[0].as<T>();
    p_object.y = p_node[1].as<T>();
    if constexpr (N > 2) {
        p_object.z = p_node[2].as<T>();
    }
    if constexpr (N > 3) {
        p_object.w = p_node[3].as<T>();
    }
    return Result<void>();
}

Result<void> SerializeYaml(YAML::Emitter& p_out, const AABB& p_object, SerializeYamlContext& p_context) {
    if (!p_object.IsValid()) {
        p_out << YAML::Null;
        return Result<void>();
    }

    p_out << YAML::BeginMap;
    p_out << YAML::Key << "min" << YAML::Value;
    SerializeYaml(p_out, p_object.GetMin(), p_context);
    p_out << YAML::Key << "max" << YAML::Value;
    SerializeYaml(p_out, p_object.GetMax(), p_context);
    p_out << YAML::EndMap;
    return Result<void>();
}

Result<void> DeserializeYaml(const YAML::Node& p_node, AABB& p_object, SerializeYamlContext& p_context) {
    if (!p_node || p_node.IsNull()) {
        p_object.MakeInvalid();
        return Result<void>();
    }

    Vector3f min, max;
    if (auto res = DeserializeYaml(p_node["min"], min, p_context); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = DeserializeYaml(p_node["max"], max, p_context); !res) {
        return HBN_ERROR(res.error());
    }

    p_object = AABB(min, max);
    return Result<void>();
}

template<HasMetaTable T>
Result<void> SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
    const auto& meta = MetaDataTable<T>::GetFields();
    DEV_ASSERT_MSG(!meta.empty(), "Did you forget to call RegisterClasses()?");

    p_out << YAML::BeginMap;

    for (const auto& field : meta) {
        SerializeYamlContext context = p_context;
        context.flags = field->flags;
        if (auto res = field->DumpYaml(p_out, &p_object, p_context); !res) {
            return HBN_ERROR(res.error());
        }
    }

    p_out << YAML::EndMap;
    return Result<void>();
}

template<HasMetaTable T>
Result<void> DeserializeYaml(const YAML::Node& p_node, T& p_object, SerializeYamlContext& p_context) {
    const auto& meta = MetaDataTable<T>::GetFields();
    DEV_ASSERT_MSG(!meta.empty(), "Did you forget to call RegisterClass()?");

    for (const auto& field : meta) {
        LOG_OK("field '{}': type: '{}', offset: '{}'", field->name, field->type, field->offset);
    }

    for (const auto& field : meta) {
        SerializeYamlContext context = p_context;
        if (auto res = field->UndumpYaml(p_node, &p_object, context); !res) {
            return HBN_ERROR(res.error());
        }
    }
    return Result<void>();
}

Result<void> SerializeYaml(YAML::Emitter& p_out, const std::string& p_object, SerializeYamlContext&) {
    p_out << p_object;
    return Result<void>();
}

Result<void> DeserializeYaml(const YAML::Node& p_node, std::string& p_object, SerializeYamlContext&) {
    if (!p_node) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Not defined");
    }

    if (!p_node.IsScalar()) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Expect scalar");
    }

    p_object = p_node.as<std::string>();
    return Result<void>();
}

template<typename T>
Result<void> FieldMeta<T>::DumpYaml(YAML::Emitter& p_out, const void* p_object, SerializeYamlContext& p_context) const {
    const T& data = FieldMetaBase::GetData<T>(p_object);
    p_out << YAML::Key << name << YAML::Value;
    if (flags & FieldFlag::EMIT_SAME_LINE) {
        p_out.SetSeqFormat(YAML::Flow);
    } else {
        p_out.SetSeqFormat(YAML::Block);
    }

    auto context = p_context;
    context.flags = flags;
    return SerializeYaml(p_out, data, context);
}

template<typename T>
Result<void> FieldMeta<T>::UndumpYaml(const YAML::Node& p_node, void* p_object, SerializeYamlContext& p_context) {
    const auto& field = p_node[name];
    const bool nuallable = flags & FieldFlag::NUALLABLE;
    if (!field && !nuallable) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "missing '{}'", name);
    }
    if (nuallable && !field) {
        return Result<void>();
    }

    T& data = FieldMetaBase::GetData<T>(p_object);
    auto context = p_context;
    context.flags = flags;
    return DeserializeYaml(field, data, context);
}

static constexpr char BIN_GUARD_MAGIC[] = "SEETHIS";

static inline Result<void> FileWrite(FileAccess* p_file, const void* p_data, size_t p_length) {
    const size_t written = p_file->WriteBuffer(p_data, p_length);
    if (written != p_length) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CANT_WRITE, "failed to write {} bytes, only wrote {}", p_length, written);
    }
    return Result<void>();
}

template<TriviallyCopyable T>
static inline Result<void> FileWrite(FileAccess* p_file, const T& p_data) {
    return FileWrite(p_file, &p_data, sizeof(T));
}

static inline Result<void> FileRead(FileAccess* p_file, void* p_data, size_t p_length) {
    const size_t read = p_file->ReadBuffer(p_data, p_length);
    if (read != p_length) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CANT_READ, "failed to read {} bytes, only read {}", p_length, read);
    }
    return Result<void>();
}

template<TriviallyCopyable T>
static inline Result<void> FileRead(FileAccess* p_file, T& p_data) {
    return FileRead(p_file, &p_data, sizeof(T));
}

template<IsVector T>
Result<void> SerializaYamlVec(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
    DEV_ASSERT(!(p_context.flags & FieldFlag::BINARY));
    const size_t count = p_object.size();
    p_out.SetSeqFormat(YAML::Flow);
    p_out << YAML::BeginSeq;
    for (size_t i = 0; i < count; ++i) {
        if (auto res = SerializeYaml(p_out, p_object[i], p_context); !res) {
            return HBN_ERROR(res.error());
        }
    }
    p_out << YAML::EndSeq;
    p_out.SetSeqFormat(YAML::Block);
    return Result<void>();
}

template<IsVector T>
Result<void> SerializaYamlVecBin(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
    DEV_ASSERT(p_context.flags & FieldFlag::BINARY);
    auto& file = p_context.file;
    DEV_ASSERT(file);

    const size_t count = p_object.size();
    const size_t size_in_byte = sizeof(p_object[0]) * count;

    int offset = 0;
    if (size_in_byte > 0) {
        if (auto res = FileWrite(file, BIN_GUARD_MAGIC); !res) {
            return HBN_ERROR(res.error());
        }
        if (auto res = FileWrite(file, size_in_byte); !res) {
            return HBN_ERROR(res.error());
        }
        // @TODO: better file API
        offset = file->Tell();
        DEV_ASSERT(offset > 0);
        if (auto res = FileWrite(file, p_object.data(), size_in_byte); !res) {
            return HBN_ERROR(res.error());
        }
    }

    p_out.SetMapFormat(YAML::Flow);
    p_out << YAML::BeginMap;
    p_out << YAML::Key << "offset" << YAML::Value << offset;
    p_out << YAML::Key << "buffer_size" << YAML::Value << size_in_byte;
    p_out << YAML::EndMap;
    p_out.SetMapFormat(YAML::Block);
    return Result<void>();
}

template<IsVector T>
Result<void> SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
    return (p_context.flags & FieldFlag::BINARY) ? SerializaYamlVecBin(p_out, p_object, p_context) : SerializaYamlVec(p_out, p_object, p_context);
}

template<IsVector T>
Result<void> DeserializeYamlVec(const YAML::Node& p_node, T& p_object, SerializeYamlContext& p_context) {
    DEV_ASSERT(!(p_context.flags & FieldFlag::BINARY));
    if (!p_node || !p_node.IsSequence()) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "not a valid sequence");
    }

    const size_t count = p_node.size();
    p_object.resize(count);
    for (size_t i = 0; i < count; ++i) {
        if (auto res = DeserializeYaml(p_node[i], p_object[i], p_context); !res) {
            return HBN_ERROR(res.error());
        }
    }
    return Result<void>();
}

template<IsVector T>
Result<void> DeserializeYamlVecBin(const YAML::Node& p_node, T& p_object, SerializeYamlContext& p_context) {
    DEV_ASSERT(p_context.flags & FieldFlag::BINARY);
    constexpr size_t element_size = sizeof(p_object[0]);
    constexpr size_t internal_offset = sizeof(BIN_GUARD_MAGIC) + sizeof(size_t);
    if (!p_node || !p_node.IsMap()) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "not a valid buffer, expect (length, offset)");
    }

    auto& file = p_context.file;
    DEV_ASSERT(file);

    size_t offset = 0;
    size_t size_in_byte = 0;
    if (auto res = DeserializeYaml(p_node["offset"], offset, p_context); !res) {
        return HBN_ERROR(res.error());
    }
    if (auto res = DeserializeYaml(p_node["buffer_size"], size_in_byte, p_context); !res) {
        return HBN_ERROR(res.error());
    }
    if (size_in_byte == 0) {
        return Result<void>();
    }
    DEV_ASSERT(size_in_byte % element_size == 0);
    DEV_ASSERT(offset >= internal_offset);

    const size_t element_count = size_in_byte / element_size;

    if (auto seek = file->Seek((long)(offset - internal_offset)); seek != 0) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CANT_READ, "Seek failed");
    }

    char magic[sizeof(BIN_GUARD_MAGIC)];
    if (auto res = FileRead(file, magic); !res) {
        return HBN_ERROR(res.error());
    }

    if (!StringUtils::StringEqual(magic, BIN_GUARD_MAGIC)) {
        magic[sizeof(BIN_GUARD_MAGIC) - 1] = 0;
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "wrong magic {}", magic);
    }

    size_t stored_length = 0;
    if (auto res = FileRead(file, stored_length); !res) {
        return HBN_ERROR(res.error());
    }

    if (stored_length != size_in_byte) {
        return HBN_ERROR(ErrorCode::ERR_FILE_CORRUPT, "wrong size (cache: {})", stored_length);
    }

    p_object.resize(element_count);
    if (auto res = FileRead(file, p_object.data(), size_in_byte); !res) {
        return HBN_ERROR(res.error());
    }

    return Result<void>();
}

template<IsVector T>
Result<void> DeserializeYaml(const YAML::Node& p_node, T& p_object, SerializeYamlContext& p_context) {
    return (p_context.flags & FieldFlag::BINARY) ? DeserializeYamlVecBin(p_node, p_object, p_context) : DeserializeYamlVec(p_node, p_object, p_context);
}

#if 0
template<typename T>
bool UndumpVectorBinary(const YAML::Node& p_node, std::vector<T>& p_value, FileAccess* p_binary) {
    // can have undefind value
    if (!p_node) {
        return true;
    }

    return true;
}
#endif

}  // namespace my::serialize
