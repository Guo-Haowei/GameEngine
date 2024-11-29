#pragma once

namespace my {

#if USING(PLATFORM_WINDOWS)
#define DELIMITER_CHAR '\\'
#define DELIMITER_STR  "\\"
#else
#define DELIMITER_CHAR '/'
#define DELIMITER_STR  "/"
#endif

class StringSplitter {
public:
    explicit StringSplitter(const char* p_string) {
        m_fast = p_string;
        m_slow = nullptr;
    }

    [[nodiscard]] std::string_view Advance(char c) {
        m_slow = m_fast;
        m_fast = strchr(m_fast, c);
        if (m_fast != nullptr) {
            return std::string_view(m_slow, m_fast++);
        }
        return m_slow;
    }

    bool CanAdvance() const {
        return m_fast != nullptr && *m_fast != 0;
    }

private:
    const char* m_fast;
    const char* m_slow;
};

class StringUtils {
public:
    static bool StringEqual(const char* p_str1, const char* p_str2);

    static void ReplaceFirst(std::string& p_string, std::string_view p_pattern, std::string_view p_replacement);

    static char* Strdup(const char* p_source);

    template<int N>
    static int Sprintf(char (&p_buffer)[N], const char* p_format, ...) {
        va_list args;
        va_start(args, p_format);
        int result = vsnprintf(p_buffer, N, p_format, args);
        va_end(args);
        return result;
    }

    template<size_t N>
    static char* Strcpy(char (&p_buffer)[N], const std::string& p_string) {
        char* result = strncpy(p_buffer, p_string.c_str(), N);
        p_buffer[N - 1] = '\0';
        return result;
    }

    static constexpr bool IsDigit(const char p_char) {
        return p_char >= '0' && p_char <= '9';
    }

    static constexpr bool IsHex(char p_char) {
        return IsDigit(p_char) || (p_char >= 'a' && p_char <= 'f') || (p_char >= 'A' && p_char <= 'F');
    }

    static constexpr char HexToInt(char p_char) {
        if (IsDigit(p_char)) {
            return p_char - '0';
        }
        if (p_char >= 'a' && p_char <= 'f') {
            return p_char - 'a' + 10;
        }
        if (p_char >= 'A' && p_char <= 'F') {
            return p_char - 'A' + 10;
        }
        return -1;
    }

    static constexpr std::string_view FindLastOf(std::string_view p_string, char p_pattern) {
        const size_t found = p_string.find_last_of(p_pattern);
        return found == std::string_view::npos ? "" : std::string_view(p_string.data() + found, p_string.size() - found);
    }

    static constexpr std::string_view BasePath(std::string_view p_string, char p_delimiter = DELIMITER_CHAR) {
        const size_t found = p_string.find_last_of(p_delimiter);
        return found == std::string_view::npos ? "" : std::string_view(p_string.data(), found);
    }

    static constexpr std::string_view FileName(std::string_view p_string, char p_delimiter = DELIMITER_CHAR) {
        auto result = FindLastOf(p_string, p_delimiter);
        return result == "" ? p_string : std::string_view(result.data() + 1, result.size() - 1);
    }

    static constexpr std::string_view Extension(std::string_view p_string) {
        return FindLastOf(p_string, '.');
    }
};

}  // namespace my
