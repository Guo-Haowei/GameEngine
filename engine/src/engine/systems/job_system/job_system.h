#pragma once

#define ENABLE_JOB_SYSTEM USE_IF(!USING(PLATFORM_WASM))

namespace my::jobsystem {

bool Initialize();

void Finalize();

class Context;

struct JobArgs {
    uint32_t jobIndex;
    uint32_t groupId;
    uint32_t groupIndex;
};

struct Job {
    Context* ctx;
    std::function<void(JobArgs)> task;
    uint32_t groupId;
    uint32_t groupJobOffset;
    uint32_t groupJobEnd;
};

class Context {
public:
#if USING(ENABLE_JOB_SYSTEM)
    void DecreaseTaskCount() { m_taskCount.fetch_sub(1); }

    bool IsBusy() const { return m_taskCount.load() > 0; }

    void Dispatch(uint32_t p_job_count, uint32_t p_group_size, const std::function<void(JobArgs)>& p_task);

    void Wait();

private:
    std::atomic_int m_taskCount = 0;
#else
    void Wait() {}
#endif
};

void WorkerMain();

}  // namespace my::jobsystem
