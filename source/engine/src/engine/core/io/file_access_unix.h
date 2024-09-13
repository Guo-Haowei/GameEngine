#pragma once
#include "file_access.h"

namespace my {

class FileAccessUnix : public FileAccess {
public:
    ~FileAccessUnix();

    void close() override;
    bool isOpen() const override;
    size_t getLength() const override;
    bool readBuffer(void* p_data, size_t p_size) const override;
    bool writeBuffer(const void* p_data, size_t p_size) override;

protected:
    ErrorCode openInternal(std::string_view p_path, ModeFlags p_mode_flags) override;

    FILE* m_file_handle{ nullptr };
};

}  // namespace my
