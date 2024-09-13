#include "file_path.h"

namespace my {

FilePath::FilePath(const std::string& p_path) : m_path(p_path) {
    prettify();
}

FilePath::FilePath(std::string_view p_path) : m_path(p_path) {
    prettify();
}

FilePath::FilePath(const char* p_path) : m_path(p_path) {
    prettify();
}

void FilePath::prettify() {
    for (char& c : m_path) {
        if (c == '\\') {
            c = '/';
        }
    }
}

}  // namespace my
