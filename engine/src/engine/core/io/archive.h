#pragma once
#include "file_access.h"

namespace my {

class Archive {
public:
    ~Archive() {
        Close();
    }

    [[nodiscard]] auto OpenRead(const std::string& p_path) -> Result<void> { return OpenMode(p_path.c_str(), false); }
    [[nodiscard]] auto OpenWrite(const std::string& p_path) -> Result<void> { return OpenMode(p_path.c_str(), true); }

    void Close();
    bool IsWriteMode() const;
    bool IsReadMode() const;

    bool Write(const void* p_data, size_t p_size);
    bool Read(void* p_data, size_t p_size);

    template<typename T>
    bool Write(const T& p_value) {
        const size_t written = m_file->Write(p_value);
        return written == sizeof(T);
    }

    template<typename T>
    bool Read(T& p_value) {
        const size_t read = m_file->Read(p_value);
        return read == sizeof(T);
    }

    template<>
    bool Write(const std::string& p_value) {
        return WriteString(p_value.data(), p_value.length());
    }

    template<>
    bool Read(std::string& p_value) {
        uint64_t length = 0;
        if (!Read(length)) {
            return false;
        }
        p_value.resize(length);
        return Read(p_value.data(), length);
    }

    template<typename T>
        requires TriviallyCopyable<T>
    bool Write(const std::vector<T>& p_value) {
        const uint64_t size = p_value.size();
        bool ok = Write(size);
        ok = ok && Write(p_value.data(), sizeof(T) * size);
        return ok;
    }

    template<typename T>
        requires TriviallyCopyable<T>
    bool Read(std::vector<T>& p_value) {
        uint64_t size = 0;
        if (!Read(size)) {
            return false;
        }
        p_value.resize(size);
        return Read(p_value.data(), sizeof(T) * size);
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator>>(std::vector<T>& p_value) {
        uint64_t size = 0;
        Read(size);
        p_value.resize(size);
        Read(p_value.data(), sizeof(T) * size);
        return *this;
    }

    template<typename T>
    Archive& operator<<(const T& p_value) {
        Write(p_value);
        return *this;
    }

    template<typename T>
    Archive& operator>>(T& p_value) {
        Read(p_value);
        return *this;
    }

    std::shared_ptr<FileAccess>& GetFileAccess() { return m_file; }

    template<typename T>
    void ArchiveValue(T& p_value) {
        if (m_isWriteMode) {
            Write(p_value);
        } else {
            Read(p_value);
        }
    }

private:
    [[nodiscard]] auto OpenMode(const std::string& p_path, bool p_write_mode) -> Result<void>;

    bool WriteString(const char* p_data, size_t p_length);

    bool m_isWriteMode{ false };
    std::shared_ptr<FileAccess> m_file{};
    std::string m_path{};
};

}  // namespace my
