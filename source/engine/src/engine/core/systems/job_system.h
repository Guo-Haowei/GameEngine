#pragma once

namespace my::jobsystem {

struct JobArgs {
    uint32_t job_index;
    uint32_t group_id;
    uint32_t group_index;
};

class Context {
public:
    void addTaskCount(int p_num) { m_task_count.fetch_add(p_num); }

    bool isBusy() const { return m_task_count.load() > 0; }

    void dispatch(uint32_t p_job_count, uint32_t p_group_size, const std::function<void(JobArgs)>& p_task);

    void wait();

private:
    std::atomic_int m_task_count = 0;
};

struct Job {
    Context* ctx;
    std::function<void(JobArgs)> task;
    uint32_t group_id;
    uint32_t group_job_offset;
    uint32_t group_job_end;
};

bool initialize();

void finalize();

void workerMain();

}  // namespace my::jobsystem
