#pragma once
#include <yaml-cpp/yaml.h>

#include "engine/core/io/file_access.h"
#include "engine/core/math/aabb.h"
#include "engine/core/math/angle.h"
#include "engine/core/string/string_utils.h"
#include "engine/systems/ecs/entity.h"

namespace my::serialize {

template<typename T>
concept IsArithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept IsEnum = std::is_enum_v<T>;

template<typename T>
concept IsVector = std::same_as<std::remove_cv_t<T>, std::vector<typename T::value_type>>;

template<typename T>
concept HasMetaTable = requires(T) {
    { T::RegisterClass() } -> std::same_as<void>;
};

Result<void> SerializeYaml(YAML::Emitter& p_out, const ecs::Entity& p_object, SerializeYamlContext&);

Result<void> DeserializeYaml(const YAML::Node& p_node, ecs::Entity& p_object, SerializeYamlContext&);

Result<void> SerializeYaml(YAML::Emitter& p_out, const Degree& p_object, SerializeYamlContext&);

Result<void> DeserializeYaml(const YAML::Node& p_node, Degree& p_object, SerializeYamlContext&);

Result<void> SerializeYaml(YAML::Emitter& p_out, const AABB& p_object, SerializeYamlContext& p_context);

Result<void> DeserializeYaml(const YAML::Node& p_node, AABB& p_object, SerializeYamlContext& p_context);

Result<void> SerializeYaml(YAML::Emitter& p_out, const std::string& p_object, SerializeYamlContext&);

Result<void> DeserializeYaml(const YAML::Node& p_node, std::string& p_object, SerializeYamlContext&);

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

template<IsEnum T>
Result<void> SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext&) {
    p_out << std::to_underlying(p_object);
    return Result<void>();
}

template<IsEnum T>
Result<void> DeserializeYaml(const YAML::Node& p_node, T& p_object, SerializeYamlContext&) {
    if (!p_node) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Not defined");
    }

    if (!p_node.IsScalar()) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_DATA, "Expect scalar");
    }

    p_object = static_cast<T>(p_node.as<uint64_t>());
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
        SerializeYamlContext context = p_context;
        if (auto res = field->UndumpYaml(p_node, &p_object, context); !res) {
            return HBN_ERROR(res.error());
        }
    }
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
Result<void> SerializaYamlVecBinary(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
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
    return (p_context.flags & FieldFlag::BINARY) ? SerializaYamlVecBinary(p_out, p_object, p_context) : SerializaYamlVec(p_out, p_object, p_context);
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
Result<void> DeserializeYamlVecBinary(const YAML::Node& p_node, T& p_object, SerializeYamlContext& p_context) {
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
    return (p_context.flags & FieldFlag::BINARY) ? DeserializeYamlVecBinary(p_node, p_object, p_context) : DeserializeYamlVec(p_node, p_object, p_context);
}

}  // namespace my::serialize
