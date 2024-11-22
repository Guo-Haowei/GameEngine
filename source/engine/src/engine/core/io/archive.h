#pragma once
#include "file_access.h"

namespace my {

class Archive {
public:
    ~Archive() {
        Close();
    }

    [[nodiscard]] auto OpenRead(const std::string& p_path) { return OpenMode(p_path.c_str(), false); }
    [[nodiscard]] auto OpenWrite(const std::string& p_path) { return OpenMode(p_path.c_str(), true); }

    void Close();
    bool IsWriteMode() const;

    Archive& operator<<(const char* p_value) {
        return WriteString(p_value, strlen(p_value));
    }

    Archive& operator<<(const std::string& p_value) {
        return WriteString(p_value.data(), p_value.length());
    }

    Archive& operator>>(std::string& p_value) {
        size_t string_length = 0;
        Read(string_length);
        p_value.resize(string_length);
        Read(p_value.data(), string_length);
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator<<(const std::vector<T>& p_value) {
        uint64_t size = p_value.size();
        Write(size);
        Write(p_value.data(), sizeof(T) * size);
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator>>(std::vector<T>& p_value) {
        uint64_t size = 0;
        Read(size);
        p_value.resize(size);
        Read(p_value.data(), sizeof(T) * size);
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator<<(const T& p_value) {
        Write(&p_value, sizeof(T));
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator>>(T& p_value) {
        Read(&p_value, sizeof(T));
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool Write(const T& p_value) {
        return Write(&p_value, sizeof(p_value));
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool Read(T& p_value) {
        return Read(&p_value, sizeof(p_value));
    }

    bool Write(const void* p_data, size_t p_size);
    bool Read(void* p_data, size_t p_size);

private:
    [[nodiscard]] auto OpenMode(const std::string& p_path, bool p_write_mode) -> Result<void>;

    Archive& WriteString(const char* p_data, size_t p_length);

    bool m_isWriteMode{ false };
    std::shared_ptr<FileAccess> m_file{};
    std::string m_path{};
};

}  // namespace my
