#pragma once
#include "file_access.h"

namespace my {

class FileAccessUnix : public FileAccess {
public:
    ~FileAccessUnix();

    void Close() override;
    bool IsOpen() const override;
    size_t GetLength() const override;
    size_t ReadBuffer(void* p_data, size_t p_size) const override;
    size_t WriteBuffer(const void* p_data, size_t p_size) override;
    long Tell() override;
    int Seek(long p_offset) override;

protected:
    auto OpenInternal(std::string_view p_path, ModeFlags p_mode_flags) -> Result<void> override;

    FILE* m_fileHandle{ nullptr };
};

}  // namespace my
