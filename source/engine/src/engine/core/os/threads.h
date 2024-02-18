#pragma once

namespace my::thread {

using ThreadMainFunc = std::function<void()>;

enum ThreadID : uint32_t {
    THREAD_MAIN = 0,
    THREAD_ASSET_LOADER_0,
    THREAD_ASSET_LOADER_1,
    THREAD_ASSET_LOADER_2,
    THREAD_JOB_SYSTEM_WORKER_0,
    THREAD_JOB_SYSTEM_WORKER_1,
    THREAD_JOB_SYSTEM_WORKER_2,
    THREAD_JOB_SYSTEM_WORKER_3,
    THREAD_JOB_SYSTEM_WORKER_4,
    THREAD_JOB_SYSTEM_WORKER_5,
    THREAD_JOB_SYSTEM_WORKER_6,
    THREAD_JOB_SYSTEM_WORKER_7,
    THREAD_MAX,
};

bool initialize();

void finailize();

bool is_shutdown_requested();

void request_shutdown();

bool is_main_thread();

uint32_t get_thread_id();

}  // namespace my::thread
