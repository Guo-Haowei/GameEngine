#pragma once
#include "file_access.h"

namespace my {

class Archive {
public:
    ~Archive() {
        close();
    }

    [[nodiscard]] auto openRead(const std::string& p_path) { return openMode(p_path.c_str(), false); }
    [[nodiscard]] auto openWrite(const std::string& p_path) { return openMode(p_path.c_str(), true); }

    void close();
    bool isWriteMode() const;

    Archive& operator<<(const char* p_value) {
        return writeString(p_value, strlen(p_value));
    }

    Archive& operator<<(const std::string& p_value) {
        return writeString(p_value.data(), p_value.length());
    }

    Archive& operator>>(std::string& p_value) {
        size_t string_length = 0;
        read(string_length);
        p_value.resize(string_length);
        read(p_value.data(), string_length);
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator<<(const std::vector<T>& p_value) {
        uint64_t size = p_value.size();
        write(size);
        write(p_value.data(), sizeof(T) * size);
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator>>(std::vector<T>& p_value) {
        uint64_t size = 0;
        read(size);
        p_value.resize(size);
        read(p_value.data(), sizeof(T) * size);
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator<<(const T& p_value) {
        write(&p_value, sizeof(T));
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    Archive& operator>>(T& p_value) {
        read(&p_value, sizeof(T));
        return *this;
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool write(const T& p_value) {
        return write(&p_value, sizeof(p_value));
    }

    template<typename T, class = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool read(T& p_value) {
        return read(&p_value, sizeof(p_value));
    }

    bool write(const void* p_data, size_t p_size);
    bool read(void* p_data, size_t p_size);

private:
    [[nodiscard]] auto openMode(const std::string& p_path, bool p_write_mode) -> std::expected<void, Error<ErrorCode>>;

    Archive& writeString(const char* p_data, size_t p_length);

    bool m_write_mode{ false };
    std::shared_ptr<FileAccess> m_file{};
    std::string m_path{};
};

}  // namespace my
