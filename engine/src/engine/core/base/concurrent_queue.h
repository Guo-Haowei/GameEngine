#pragma once

namespace my {

// @TODO: check out implmentation here
// https://github.com/supermartian/lockfree-queue/tree/master
// https://people.cs.pitt.edu/~jacklange/teaching/cs2510-f12/papers/implementing_lock_free.pdf
template<typename T>
class ConcurrentQueue {
public:
    void push(const T& p_value) {
        m_mutex.lock();
        m_queue.emplace(p_value);
        m_mutex.unlock();
    }

    bool pop(T& p_out_value) {
        std::lock_guard guard(m_mutex);
        if (m_queue.empty()) {
            return false;
        }

        p_out_value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    std::queue<T> pop_all() {
        std::queue<T> ret;
        m_mutex.lock();
        if (!m_queue.empty()) {
            m_queue.swap(ret);
        }
        m_mutex.unlock();
        return ret;
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
};

}  // namespace my
