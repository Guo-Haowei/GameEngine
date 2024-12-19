#pragma once
#include "serialization.h"

namespace my::serialize {

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

}  // namespace my::serialize
