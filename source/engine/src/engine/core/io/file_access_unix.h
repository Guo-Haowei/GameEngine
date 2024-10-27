#pragma once
#include "file_access.h"

namespace my {

class FileAccessUnix : public FileAccess {
public:
    ~FileAccessUnix();

    void Close() override;
    bool IsOpen() const override;
    size_t GetLength() const override;
    bool ReadBuffer(void* p_data, size_t p_size) const override;
    bool WriteBuffer(const void* p_data, size_t p_size) override;

protected:
    ErrorCode OpenInternal(std::string_view p_path, ModeFlags p_mode_flags) override;

    FILE* m_fileHandle{ nullptr };
};

}  // namespace my
