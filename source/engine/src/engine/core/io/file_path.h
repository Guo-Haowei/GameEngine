#pragma once

namespace my {

class FilePath {
public:
    FilePath() = default;
    explicit FilePath(const std::string& p_path);
    explicit FilePath(std::string_view p_path);
    explicit FilePath(const char* p_path);

    const std::string& string() const { return m_path; }
    const char* extensionCStr() const;

    std::string extension() const {
        return std::string(extensionCStr());
    }

    FilePath operator/(const char* p_path) const { return concat(p_path); }
    FilePath operator/(std::string_view p_path) const { return concat(p_path); }
    FilePath operator/(const std::string& p_path) const { return concat(p_path); }
    FilePath operator/(const FilePath& p_path) const { return concat(p_path.string()); }

    bool operator==(const FilePath& p_other) const {
        return m_path == p_other.m_path;
    }

    bool operator<(const FilePath& p_other) const {
        return m_path < p_other.m_path;
    }

private:
    void prettify();
    FilePath concat(std::string_view p_path) const;

    std::string m_path;
};

}  // namespace my

namespace std {

template<>
struct hash<my::FilePath> {
    std::size_t operator()(const my::FilePath& p_file_path) const { return std::hash<std::string>{}(p_file_path.string()); }
};

}  // namespace std
