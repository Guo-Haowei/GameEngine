#pragma once
#include "file_access_unix.h"

namespace my {

FileAccessUnix::~FileAccessUnix() { close(); }

ErrorCode FileAccessUnix::openInternal(std::string_view p_path, int p_mode_flags) {
    ERR_FAIL_COND_V(m_file_handle, ERR_FILE_ALREADY_IN_USE);

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
    m_file_handle = fopen(path_string.c_str(), mode);

    if (!m_file_handle) {
        switch (errno) {
            case ENOENT:
                return ERR_FILE_NOT_FOUND;
            default:
                return ERR_FILE_CANT_OPEN;
        }
    }

    return OK;
}

void FileAccessUnix::close() {
    if (m_file_handle) {
        fclose(m_file_handle);
        m_file_handle = nullptr;
    }
}

bool FileAccessUnix::isOpen() const {
    return m_file_handle != nullptr;
}

size_t FileAccessUnix::getLength() const {
    fseek(m_file_handle, 0, SEEK_END);
    const size_t length = ftell(m_file_handle);
    fseek(m_file_handle, 0, SEEK_SET);
    return length;
}

bool FileAccessUnix::readBuffer(void* p_data, size_t p_size) const {
    DEV_ASSERT(isOpen());

    if (fread(p_data, 1, p_size, m_file_handle) != p_size) {
        return false;
    }

    return true;
}

bool FileAccessUnix::writeBuffer(const void* p_data, size_t p_size) {
    DEV_ASSERT(isOpen());

    if (fwrite(p_data, 1, p_size, m_file_handle) != p_size) {
        return false;
    }

    return true;
}

}  // namespace my
