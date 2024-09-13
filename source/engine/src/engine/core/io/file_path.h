#pragma once

namespace my {

class FilePath {
public:
    FilePath() = default;
    explicit FilePath(const std::string& p_path);
    explicit FilePath(std::string_view p_path);
    explicit FilePath(const char* p_path);

    // @TODO: remove
    const std::string& getString() const { return m_path; }

    // @TODO: custom hash
    // @TODO: custom fmt
    // @TODO: fs::path{ ROOT_FOLDER } / "source" / "shader" / p_file;
private:
    void prettify();

    std::string m_path;
};

}  // namespace my
