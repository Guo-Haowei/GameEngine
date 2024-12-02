#include "threads.h"

#include <latch>
#include <thread>

#include "engine/core/framework/asset_manager.h"
#include "engine/core/io/print.h"
#include "engine/core/systems/job_system.h"
#include "engine/drivers/windows/win32_prerequisites.h"

namespace my::thread {

using ThreadMainFunc = void (*)();

struct ThreadObject {
    const char* name;
    ThreadMainFunc threadFunc;
    uint32_t id{ 0 };
    std::thread threadObject{};
};

static thread_local uint32_t g_threadId;
static struct {
    std::atomic_bool shutdownRequested;
    std::array<ThreadObject, THREAD_MAX> threads = {
#define THREAD_DEFINE(ENUM, FUNC) ThreadObject{ #ENUM, FUNC },
        THREAD_LIST
#undef THREAD_DEFINE
    };
} s_threadGlob;

bool Initialize() {
    g_threadId = THREAD_MAIN;

    std::latch latch{ THREAD_MAX - 1 };

    for (uint32_t id = THREAD_MAIN + 1; id < THREAD_MAX; ++id) {
        ThreadObject& thread = s_threadGlob.threads[id];
        thread.id = id;
        thread.threadObject = std::thread(
            [&](ThreadObject* p_object) {
                // set thread id
                g_threadId = p_object->id;

                latch.count_down();
                LOG_VERBOSE("[threads] thread '{}'(id: {}) starts.", p_object->name, p_object->id);
                p_object->threadFunc();
                LOG_VERBOSE("[threads] thread '{}'(id: {}) ends.", p_object->name, p_object->id);
            },
            &thread);

#if USING(PLATFORM_WINDOWS)
        HANDLE handle = (HANDLE)thread.threadObject.native_handle();

        // @TODO: set thread affinity
        // DWORD_PTR affinityMask = 1ull << threadID;
        // DWORD_PTR affinityResult = SetThreadAffinityMask(handle, affinityMask);

        std::string name = thread.name;
        std::wstring wname(name.begin(), name.end());
        HRESULT hr = SetThreadDescription(handle, wname.c_str());
        DEV_ASSERT(!FAILED(hr));
#endif
    }

    latch.wait();
    return true;
}

void Finailize() {
    for (uint32_t id = THREAD_MAIN + 1; id < THREAD_MAX; ++id) {
        auto& thread = s_threadGlob.threads[id].threadObject;
        if (thread.joinable()) {
            thread.join();
        }
    }
}

bool ShutdownRequested() {
    return s_threadGlob.shutdownRequested;
}

void RequestShutdown() {
    s_threadGlob.shutdownRequested = true;
}

bool IsMainThread() {
    return g_threadId == THREAD_MAIN;
}

uint32_t GetThreadId() {
    return g_threadId;
}

}  // namespace my::thread
