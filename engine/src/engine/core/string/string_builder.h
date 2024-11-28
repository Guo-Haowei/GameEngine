#pragma once

namespace my {

class IStringBuilder {
public:
    // @TODO: redesign the interface
    virtual void Append(char p_value) = 0;
    virtual void Append(const char* p_value) = 0;
    virtual void Append(const std::string& p_value) = 0;
    virtual void Append(std::string_view p_value) = 0;

    virtual std::string ToString() = 0;
};

class StringStreamBuilder : public IStringBuilder {
public:
    void Append(char p_value) override {
        m_stream << p_value;
    }
    void Append(const char* p_value) override {
        m_stream << p_value;
    }
    void Append(const std::string& p_value) override {
        m_stream << p_value;
    }
    void Append(std::string_view p_value) override {
        m_stream << p_value;
    }

    std::string ToString() override {
        return m_stream.str();
    }

protected:
    std::stringstream m_stream;
};

// @TODO: fixed string builder

}  // namespace my
