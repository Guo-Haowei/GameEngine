#pragma once
#include "file_access_unix.h"

namespace my {

FileAccessUnix::~FileAccessUnix() { Close(); }

ErrorCode FileAccessUnix::OpenInternal(std::string_view p_path, ModeFlags p_mode_flags) {
    ERR_FAIL_COND_V(m_fileHandle, ERR_FILE_ALREADY_IN_USE);

    const char* mode = "";
    switch (p_mode_flags) {
        case READ:
            mode = "rb";
            break;
        case WRITE:
            mode = "wb";
            break;
        default:
            return ERR_INVALID_PARAMETER;
    }

    std::string path_string{ p_path };
    m_fileHandle = fopen(path_string.c_str(), mode);

    if (!m_fileHandle) {
        switch (errno) {
            case ENOENT:
                return ERR_FILE_NOT_FOUND;
            default:
                return ERR_FILE_CANT_OPEN;
        }
    }

    return OK;
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

bool FileAccessUnix::ReadBuffer(void* p_data, size_t p_size) const {
    DEV_ASSERT(IsOpen());

    if (fread(p_data, 1, p_size, m_fileHandle) != p_size) {
        return false;
    }

    return true;
}

bool FileAccessUnix::WriteBuffer(const void* p_data, size_t p_size) {
    DEV_ASSERT(IsOpen());

    if (fwrite(p_data, 1, p_size, m_fileHandle) != p_size) {
        return false;
    }

    return true;
}

}  // namespace my
