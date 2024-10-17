#pragma once
#include "core/base/noncopyable.h"

namespace my {

class Layer : public NonCopyable {
public:
    Layer(const std::string& name = "Layer") : m_name(name) {}

    virtual void attach() = 0;
    virtual void render() = 0;
    virtual void update(float dt) = 0;

    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
};

}  // namespace my