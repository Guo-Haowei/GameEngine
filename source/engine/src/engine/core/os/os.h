#pragma once
#include "core/base/singleton.h"
#include "core/io/logger.h"

namespace my {

class OS : public Singleton<OS> {
public:
    void initialize();
    void finalize();

    virtual void print(LogLevel p_level, std::string_view p_message);

    void addLogger(std::shared_ptr<ILogger> p_logger);

protected:
    CompositeLogger m_logger;
};

}  // namespace my