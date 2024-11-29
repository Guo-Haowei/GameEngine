#pragma once
#include "engine/core/base/noncopyable.h"

namespace my {

class Application;

class Layer : public NonCopyable {
public:
    Layer(std::string_view p_name) : m_name(p_name) {}
    virtual ~Layer() = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}

    virtual void OnImGuiRender() {}
    virtual void OnUpdate(float p_time_step) { unused(p_time_step); }

    const std::string& GetName() const { return m_name; }
    Application* GetApplication() const { return m_app; }

protected:
    std::string m_name;

    Application* m_app{ nullptr };

    friend class Application;
};

}  // namespace my