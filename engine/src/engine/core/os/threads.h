#pragma once
#include "engine/systems/job_system/job_system.h"

namespace my::thread {

enum ThreadID : uint32_t {
    THREAD_MAIN,
    THREAD_ASSET_LOADER_1,
// THREAD_ASSET_LOADER_2,
#if USING(ENABLE_JOB_SYSTEM)
    THREAD_JOBSYSTEM_WORKER_1,
    THREAD_JOBSYSTEM_WORKER_2,
    THREAD_JOBSYSTEM_WORKER_3,
    THREAD_JOBSYSTEM_WORKER_4,
    THREAD_JOBSYSTEM_WORKER_5,
    THREAD_JOBSYSTEM_WORKER_6,
    THREAD_JOBSYSTEM_WORKER_7,
    THREAD_JOBSYSTEM_WORKER_8,
#endif
    THREAD_MAX,
};

bool Initialize();

void Finailize();

bool ShutdownRequested();

void RequestShutdown();

bool IsMainThread();

uint32_t GetThreadId();

}  // namespace my::thread
