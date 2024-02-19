#include "loader.h"

namespace my {

LoaderBase::LoaderBase(const std::string& path) {
    m_file_path = path;
    std::filesystem::path system_path{ path };
    m_file_name = system_path.filename().string();
    m_base_path = system_path.remove_filename().string();
    m_error.clear();
}

}  // namespace my
