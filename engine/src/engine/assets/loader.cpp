#include "loader.h"

namespace my {

LoaderBase::LoaderBase(const std::string& path) {
    m_filePath = path;
    std::filesystem::path system_path{ path };
    m_fileName = system_path.filename().string();
    m_basePath = system_path.remove_filename().string();
    m_error.clear();
}

}  // namespace my
