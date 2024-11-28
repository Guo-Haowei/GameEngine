#pragma once

namespace my {

class Application;

class Module {
public:
    Module(std::string_view name) : m_initialized(false), m_name(name) {}
    virtual ~Module() = default;

    auto Initialize() -> Result<void>;
    void Finalize();

    std::string_view GetName() const { return m_name; }

protected:
    virtual auto InitializeImpl() -> Result<void> = 0;
    virtual void FinalizeImpl() = 0;

    bool m_initialized{ false };
    std::string_view m_name;
    Application* m_app;
    friend class Application;
};

}  // namespace my
