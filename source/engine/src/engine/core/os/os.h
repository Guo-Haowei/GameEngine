#pragma once
#include "core/base/singleton.h"
#include "core/io/logger.h"

namespace my {

class OS : public Singleton<OS> {
public:
    void Initialize();
    void Finalize();

    virtual void Print(LogLevel p_level, std::string_view p_message);

    void AddLogger(std::shared_ptr<ILogger> p_logger);

protected:
    CompositeLogger m_logger;
};

bool IsAnsiSupported();

bool EnableAnsi();

}  // namespace my