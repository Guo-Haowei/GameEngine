#include "engine.h"

#include "engine/core/os/os.h"
#include "engine/core/os/threads.h"
#include "engine/core/systems/job_system.h"

namespace my {

static OS* s_os;

bool engine::InitializeCore() {
    if (s_os) {
        return true;
    }

    s_os = new OS;
    s_os->Initialize();

    thread::Initialize();
    jobsystem::Initialize();

    return true;
}

void engine::FinalizeCore() {
    if (!s_os) {
        return;
    }

    jobsystem::Finalize();
    thread::Finailize();

    s_os->Finalize();
    delete s_os;
    s_os = nullptr;
}

}  // namespace my
