#pragma once

namespace my {

class Guid {
public:
    explicit Guid();

    explicit Guid(const uint8_t* p_buffer);

    static Guid Create();
    static Result<Guid> Parse(const char* p_start, int p_length);
    static Result<Guid> Parse(const std::string& p_string) {
        return Parse(p_string.c_str(), static_cast<int>(p_string.length()));
    }

    bool operator==(const Guid& p_rhs) const {
        for (int i = 0; i < array_length(m_data); ++i) {
            if (m_data[i] != p_rhs.m_data[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Guid& p_rhs) const {
        return !(*this == p_rhs);
    }

    std::string ToString() const;

    const uint8_t* GetData() const {
        return m_data;
    };

private:
    uint8_t m_data[16];
};

}  // namespace my

namespace std {

template<>
struct hash<my::Guid> {
    std::size_t operator()(const my::Guid& p_guid) const {
        std::size_t hash = 0;

        const uint8_t* data = p_guid.GetData();
        // Combine hash for each byte in the buffer
        for (std::size_t i = 0; i < sizeof(my::Guid); ++i) {
            hash ^= std::hash<uint8_t>{}(data[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        return hash;
    }
};

template<>
struct less<my::Guid> {
    bool operator()(const my::Guid& p_lhs, const my::Guid& p_rhs) const {
        return memcmp(p_lhs.GetData(), p_rhs.GetData(), sizeof(my::Guid)) < 0;
    }
};

}  // namespace std
