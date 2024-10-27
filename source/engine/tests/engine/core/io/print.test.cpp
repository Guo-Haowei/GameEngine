#include "core/io/print.h"

#include "core/os/os.h"

namespace my {

class TestLogger : public ILogger {
public:
    virtual void Print(LogLevel, std::string_view message) override { m_buffer.append(message); }
    const std::string& GetBuffer() const { return m_buffer; }
    void ClearBuffer() { m_buffer.clear(); }

private:
    std::string m_buffer;
};

TEST(print, PrintImpl) {
    OS dummy_os;
    auto logger = std::make_shared<TestLogger>();
    dummy_os.AddLogger(logger);

    PrintImpl(LOG_LEVEL_ERROR, "{}, {}, {}", 1, 'c', "200");
    EXPECT_EQ(logger->GetBuffer(), "1, c, 200");
}

}  // namespace my
