#include "job_system.h"

#include "engine/core/base/thread_safe_ring_buffer.h"
#include "engine/core/os/threads.h"
#include "engine/math/geomath.h"

namespace my::jobsystem {

static struct
{
    std::condition_variable wakeCondition;
    std::mutex wakeMutex;
    ThreadSafeRingBuffer<Job, 128> jobQueue;
} s_glob;

static bool DoWork() {
    Job job;
    if (!s_glob.jobQueue.pop_front(job)) {
        return false;
    }

    for (uint32_t i = job.groupJobOffset; i < job.groupJobEnd; ++i) {
        JobArgs args;
        args.groupId = job.groupId;
        args.jobIndex = i;
        args.groupIndex = i - job.groupJobOffset;
        job.task(args);
    }

    job.ctx->DecreaseTaskCount();
    return true;
}

void WorkerMain() {
    for (;;) {
        if (thread::ShutdownRequested()) {
            break;
        }

        if (!DoWork()) {
            std::unique_lock<std::mutex> lock(s_glob.wakeMutex);
            s_glob.wakeCondition.wait(lock);
        }
    }
}

bool Initialize() {
    return true;
}

void Finalize() {
    s_glob.wakeCondition.notify_all();
}

void Context::Dispatch(uint32_t p_job_count, uint32_t p_group_size, const std::function<void(JobArgs)>& p_task) {
    // DEV_ASSERT(thread::isMainThread());

    if (p_job_count == 0 || p_group_size == 0) {
        return;
    }

    const uint32_t group_count = (p_job_count + p_group_size - 1) / p_group_size;  // make sure round up
    m_taskCount.fetch_add(group_count);

    for (uint32_t group_id = 0; group_id < group_count; ++group_id) {
        Job job;
        job.ctx = this;
        job.task = p_task;
        job.groupId = group_id;
        job.groupJobOffset = group_id * p_group_size;
        job.groupJobEnd = glm::min(job.groupJobOffset + p_group_size, p_job_count);

        while (!s_glob.jobQueue.push_back(job)) {
            // if job queue is full, notify all and let main thread do the work as well
            s_glob.wakeCondition.notify_all();
            DoWork();
        }
    }

    s_glob.wakeCondition.notify_all();
}

void Context::Wait() {
    // Wake any threads that might be sleeping:
    s_glob.wakeCondition.notify_all();

    // Waiting will also put the current thread to good use by working on an other job if it can:
    while (IsBusy()) {
        DoWork();
    }
}

}  // namespace my::jobsystem
