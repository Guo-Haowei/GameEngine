#include "engine/core/base/concurrent_queue.h"

namespace my {

TEST(concurrent_queue, test) {
    ConcurrentQueue<int> concurrent_queue;
    std::latch latch{ 1 };
    constexpr int TASK_COUNT = 104;
    std::atomic_int task_count = TASK_COUNT;
    std::thread worker([&]() {
        latch.count_down();
        for (int i = 0; i < TASK_COUNT; ++i) {
            concurrent_queue.push(i % 26);
            int need_sleep = rand() % 4 == 0;  // 25% sleep
            if (need_sleep) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            --task_count;
        }
    });

    latch.wait();
    std::string buffer;
    for (;;) {
        if (task_count <= 0) {
            break;
        }

        auto tasks = concurrent_queue.pop_all();
        while (!tasks.empty()) {
            buffer.push_back('A' + (char)tasks.front());
            tasks.pop();
        }
    }

    worker.join();

    EXPECT_EQ(buffer,
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

}  // namespace my
