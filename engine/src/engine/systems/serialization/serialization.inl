#include <yaml-cpp/yaml.h>

#include "engine/systems/serialization/serialization.h"

namespace my::serialize {

template<typename T>
concept IsArithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept IsIterable = requires(T t) {
    { std::begin(t) } -> std::same_as<typename T::iterator>;
    { std::end(t) } -> std::same_as<typename T::iterator>;
} && !std::is_same_v<T, std::string>;

template<typename T>
concept HasMetaTable = requires(T) {
    { T::RegisterClass() } -> std::same_as<void>;
};

template<IsArithmetic T>
bool SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext&) {
    p_out << p_object;
    return true;
}

template<IsArithmetic T>
bool DeserializeYaml(const YAML::Node& p_node, T& p_object, SerializeYamlContext&) {
    p_object = p_node.as<T>();
    return true;
}

bool SerializeYaml(YAML::Emitter& p_out, const ecs::Entity& p_object, SerializeYamlContext&) {
    p_out << p_object.GetId();
    return true;
}

bool DeserializeYaml(const YAML::Node& p_node, ecs::Entity& p_object, SerializeYamlContext&) {
    p_object = ecs::Entity(p_node.as<uint32_t>());
    return true;
}

bool SerializeYaml(YAML::Emitter& p_out, const Vector3f& p_object, SerializeYamlContext&) {
    p_out.SetSeqFormat(YAML::Flow);
    p_out << YAML::BeginSeq;
    p_out << p_object.x;
    p_out << p_object.y;
    p_out << p_object.z;
    p_out << YAML::EndSeq;
    return true;
}

bool DeserializeYaml(const YAML::Node& p_node, Vector3f& p_object, SerializeYamlContext&) {
    p_object.x = p_node.as<float>();
    return true;
}

bool SerializeYaml(YAML::Emitter& p_out, const Vector4f& p_object, SerializeYamlContext&) {
    p_out.SetSeqFormat(YAML::Flow);
    p_out << YAML::BeginSeq;
    p_out << p_object.x;
    p_out << p_object.y;
    p_out << p_object.z;
    p_out << p_object.w;
    p_out << YAML::EndSeq;
    return true;
}

bool DeserializeYaml(const YAML::Node& p_node, Vector4f& p_object, SerializeYamlContext&) {
    p_object.x = p_node.as<float>();
    return true;
}

template<HasMetaTable T>
bool SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
    const auto& meta = MetaDataTable<T>::GetFields();
    DEV_ASSERT_MSG(!meta.empty(), "Did you forget to call RegisterClass()?");

    p_out << YAML::BeginMap;

    for (const auto& field : meta) {
        SerializeYamlContext context = p_context;
        context.flags = field->flags;
        field->DumpYaml(p_out, &p_object, p_context);
    }

    p_out << YAML::EndMap;
    return true;
}

template<HasMetaTable T>
bool DeserializeYaml(const YAML::Node& p_node, T& p_object, SerializeYamlContext& p_context) {
    const auto& meta = MetaDataTable<T>::GetFields();
    DEV_ASSERT_MSG(!meta.empty(), "Did you forget to call RegisterClass()?");

    for (const auto& field : meta) {
        SerializeYamlContext context = p_context;
        if (!field->UndumpYaml(p_node, &p_object, context)) {
            return false;
        }
    }
    return true;
}

template<IsIterable T>
bool SerializeYaml(YAML::Emitter& p_out, const T& p_object, SerializeYamlContext& p_context) {
    p_out << YAML::BeginSeq;
    for (const auto& ele : p_object) {
        SerializeYaml(p_out, ele, p_context);
    }
    p_out << YAML::EndSeq;
    return true;
}

template<IsIterable T>
bool DeserializeYaml(const YAML::Node& p_node, T& p_object, SerializeYamlContext& p_context) {
    const size_t size = p_node.size();
    p_object.resize(size);
    for (size_t i = 0; i < size; ++i) {
        DeserializeYaml(p_node[i], p_object[i], p_context);
    }
    return true;
}

bool SerializeYaml(YAML::Emitter& p_out, const std::string& p_object, SerializeYamlContext&) {
    p_out << p_object;
    return true;
}

bool DeserializeYaml(const YAML::Node& p_node, std::string& p_object, SerializeYamlContext&) {
    // @TODO: type check
    p_object = p_node.as<std::string>();
    return true;
}
//
// template<>
// bool SerializeYaml<ecs::Entity>(YAML::Emitter& p_out, const ecs::Entity& p_object, SerializeYamlContext&) {
//    p_out << p_object.GetId();
//    return true;
//}

// template<>
// bool DeserializeYaml(const YAML::Node& p_node, ecs::Entity&& p_object, SerializeYamlContext&) {
//     // @TODO: type check
//     p_object = ecs::Entity(p_node.as<uint32_t>());
//     return true;
// }

template<typename T>
bool FieldMeta<T>::DumpYaml(YAML::Emitter& p_out, const void* p_object, SerializeYamlContext& p_context) const {
    const T& data = FieldMetaBase::GetData<T>(p_object);
    p_out << YAML::Key << name << YAML::Value;
    if (flags & FieldFlag::EMIT_SAME_LINE) {
        p_out.SetSeqFormat(YAML::Flow);
    } else {
        p_out.SetSeqFormat(YAML::Block);
    }

    return SerializeYaml(p_out, data, p_context);
}

template<typename T>
bool FieldMeta<T>::UndumpYaml(const YAML::Node& p_node, void* p_object, SerializeYamlContext& p_context) {
    const auto& field = p_node[name];
    const bool nuallable = flags & FieldFlag::NUALLABLE;
    ERR_FAIL_COND_V(!field.IsDefined() && !nuallable, false);
    if (!field.IsDefined()) {
        return true;
    }

    T& data = FieldMetaBase::GetData<T>(p_object);
    return DeserializeYaml(field, data, p_context);
}

}  // namespace my::serialize
