#include "module_registry.h"

namespace my {

template<class T1, class FALLBACK>
inline T1* CreateModule() {
    if (T1::s_createFunc) {
        return T1::s_createFunc();
    }
    return new FALLBACK;
}

class NullPhysicsManager : public IPhysicsManager {
public:
    NullPhysicsManager() : IPhysicsManager("NullPhysicsManager") {}
    virtual ~NullPhysicsManager() = default;

    virtual auto InitializeImpl() -> Result<void> { return Result<void>(); }
    virtual void FinalizeImpl() {}

    virtual void Update(Scene&) {}

    virtual void OnSimBegin(Scene&) {}
    virtual void OnSimEnd(Scene&) {}
};

IPhysicsManager* CreatePhysicsManager() {
    return CreateModule<IPhysicsManager, NullPhysicsManager>();
}

}  // namespace my
