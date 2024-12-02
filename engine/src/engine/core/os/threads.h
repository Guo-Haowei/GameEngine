#pragma once

namespace my::thread {

#define THREAD_LIST                                                 \
    THREAD_DEFINE(THREAD_MAIN, []() {})                             \
    THREAD_DEFINE(THREAD_ASSET_LOADER_1, AssetManager::WorkerMain)  \
    THREAD_DEFINE(THREAD_ASSET_LOADER_2, AssetManager::WorkerMain)  \
    THREAD_DEFINE(THREAD_JOBSYSTEM_WORKER_1, jobsystem::WorkerMain) \
    THREAD_DEFINE(THREAD_JOBSYSTEM_WORKER_2, jobsystem::WorkerMain) \
    THREAD_DEFINE(THREAD_JOBSYSTEM_WORKER_3, jobsystem::WorkerMain) \
    THREAD_DEFINE(THREAD_JOBSYSTEM_WORKER_4, jobsystem::WorkerMain) \
    THREAD_DEFINE(THREAD_JOBSYSTEM_WORKER_5, jobsystem::WorkerMain) \
    THREAD_DEFINE(THREAD_JOBSYSTEM_WORKER_6, jobsystem::WorkerMain) \
    THREAD_DEFINE(THREAD_JOBSYSTEM_WORKER_7, jobsystem::WorkerMain) \
    THREAD_DEFINE(THREAD_JOBSYSTEM_WORKER_8, jobsystem::WorkerMain)

enum ThreadID : uint32_t {
#define THREAD_DEFINE(ENUM, ...) ENUM,
    THREAD_LIST
#undef THREAD_DEFINE
        THREAD_MAX,
};

bool Initialize();

void Finailize();

bool ShutdownRequested();

void RequestShutdown();

bool IsMainThread();

uint32_t GetThreadId();

}  // namespace my::thread
