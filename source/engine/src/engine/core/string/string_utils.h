#pragma once

namespace my {

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
};

}  // namespace my
