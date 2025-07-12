#include "file_access_unix.h"

namespace my {

namespace fs = std::filesystem;

FileAccessUnix::~FileAccessUnix() { Close(); }

static const char* ResolveFlag(FileAccess::ModeFlags p_access) {
    const bool read = p_access & FileAccess::READ;
    const bool write = p_access & FileAccess::WRITE;
    const bool trunc = p_access & FileAccess::TRUNCATE;
    const bool create = p_access & FileAccess::CREATE;

    if (read && write) {
        if (trunc || create) {
            return "w+b";
        }
        return "r+b";
    }
    if (read) {
        return "rb";
    }
    if (write) {
        return "wb";
    }
    return nullptr;
}

auto FileAccessUnix::OpenInternal(std::string_view p_path, ModeFlags p_mode_flags) -> Result<void> {
    DEV_ASSERT(!m_fileHandle);

    if (p_mode_flags & EXCLUSIVE) {
        if (fs::exists(p_path)) {
            return HBN_ERROR(ErrorCode::ERR_FILE_ALREADY_IN_USE, "file '{}' already in use", p_path);
        }
    }

    const char* mode = ResolveFlag(p_mode_flags);
    if (!mode) {
        return HBN_ERROR(ErrorCode::ERR_INVALID_PARAMETER, "invalid mode");
    }

    std::string path_string{ p_path };
    m_fileHandle = fopen(path_string.c_str(), mode);
    m_openMode = p_mode_flags;

    if (!m_fileHandle) {
        switch (errno) {
            case ENOENT:
                return HBN_ERROR(ErrorCode::ERR_FILE_NOT_FOUND, "file '{}' not found", p_path);
            default:
                return HBN_ERROR(ErrorCode::ERR_FILE_CANT_OPEN, "file '{}' not found", p_path);
        }
    }

    return Result<void>();
}

void FileAccessUnix::Close() {
    if (m_fileHandle) {
        fclose(m_fileHandle);
        m_fileHandle = nullptr;
    }
}

bool FileAccessUnix::IsOpen() const {
    return m_fileHandle != nullptr;
}

size_t FileAccessUnix::GetLength() const {
    fseek(m_fileHandle, 0, SEEK_END);
    const size_t length = ftell(m_fileHandle);
    fseek(m_fileHandle, 0, SEEK_SET);
    return length;
}

size_t FileAccessUnix::ReadBuffer(void* p_data, size_t p_size) const {
    DEV_ASSERT(m_openMode & READ);

    ERR_FAIL_COND_V(!IsOpen(), 0);
    return fread(p_data, 1, p_size, m_fileHandle);
}

size_t FileAccessUnix::WriteBuffer(const void* p_data, size_t p_size) {
    DEV_ASSERT(m_openMode & WRITE);

    ERR_FAIL_COND_V(!IsOpen(), 0);
    return fwrite(p_data, 1, p_size, m_fileHandle);
}

long FileAccessUnix::Tell() {
    ERR_FAIL_COND_V(!IsOpen(), -1);
    return ftell(m_fileHandle);
}
int FileAccessUnix::Seek(long p_offset) {
    // @TODO: refactor error code
    ERR_FAIL_COND_V(!IsOpen(), -1);

    const int res = fseek(m_fileHandle, p_offset, SEEK_SET);
    if (res != 0) {
        LOG_ERROR("seek failed");
    }
    return res;
}

}  // namespace my
