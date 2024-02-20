#pragma once

namespace my {

class Archive;

class NameComponent {
public:
    NameComponent() = default;

    NameComponent(const char* tag) { m_name = tag; }

    void set_name(const char* tag) { m_name = tag; }
    void set_name(const std::string& tag) { m_name = tag; }

    const std::string& get_name() const { return m_name; }
    std::string& get_name_ref() { return m_name; }

    void serialize(Archive& archive, uint32_t version);

private:
    std::string m_name;
};

}  // namespace my
