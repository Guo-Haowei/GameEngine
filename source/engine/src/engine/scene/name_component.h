#pragma once

namespace my {

class Archive;

class NameComponent {
public:
    NameComponent() = default;

    NameComponent(const char* p_name) { m_name = p_name; }

    void setName(const char* p_name) { m_name = p_name; }
    void setName(const std::string& p_name) { m_name = p_name; }

    const std::string& getName() const { return m_name; }
    std::string& getNameRef() { return m_name; }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    std::string m_name;
};

}  // namespace my
