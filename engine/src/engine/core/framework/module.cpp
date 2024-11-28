#include "module.h"

namespace my {

auto Module::Initialize() -> Result<void> {
    if (DEV_VERIFY(!m_initialized)) {
        auto res = InitializeImpl();
        if (!res) {
            return HBN_ERROR(res.error());
        }

        m_initialized = true;
    }

    return Result<void>();
}

void Module::Finalize() {
    if (m_initialized) {
        FinalizeImpl();

        m_initialized = false;
    }
}

}  // namespace my
