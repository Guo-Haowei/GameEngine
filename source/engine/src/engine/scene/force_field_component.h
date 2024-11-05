#pragma once

namespace my {

class Archive;

struct ForceFieldComponent {
    void Serialize(Archive& p_archive, uint32_t p_version);

    float strength{ 0.0f };
    float radius{ 0.01f };
};

}  // namespace my

