#include "archive.h"

#include "core/io/print.h"

namespace my {

auto Archive::openMode(const std::string& p_path, bool p_write_mode) -> std::expected<void, Error<ErrorCode>> {
    m_path = p_path;
    m_write_mode = p_write_mode;
    if (m_write_mode) {
        m_path += ".tmp";
    }

    auto res = FileAccess::open(m_path, p_write_mode ? FileAccess::WRITE : FileAccess::READ);
    if (!res) {
        return std::unexpected(res.error());
    }

    m_file = *res;
    return std::expected<void, Error<ErrorCode>>();
}

void Archive::close() {
    if (!m_file) {
        return;
    }

    m_file.reset();

    if (m_write_mode) {
        std::filesystem::path final_path{ m_path };
        final_path.replace_extension();
        std::filesystem::rename(m_path, final_path);
    }
}

bool Archive::isWriteMode() const {
    DEV_ASSERT(m_file);
    return m_write_mode;
}

bool Archive::write(const void* p_data, size_t p_size) {
    DEV_ASSERT(m_file && m_write_mode);
    return m_file->writeBuffer(p_data, p_size);
}

bool Archive::read(void* p_data, size_t p_size) {
    DEV_ASSERT(m_file && !m_write_mode);
    return m_file->readBuffer(p_data, p_size);
}

Archive& Archive::writeString(const char* p_data, size_t p_length) {
    write(p_length);
    write(p_data, p_length);
    return *this;
}

}  // namespace my