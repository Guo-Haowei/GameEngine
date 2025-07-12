#pragma once

namespace my {

class SpinLock {
public:
    void Lock() {
        while (m_flag.test_and_set(std::memory_order_acquire)) {
        }
    }

    void Unlock() {
        m_flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
};

template<typename T>
class LockGuard {
public:
    LockGuard(T& p_lock)
        : m_lock(p_lock) {
        m_lock.lock();
    }
    ~LockGuard() {
        m_lock.unlock();
    }

protected:
    T& m_lock;
};

}  // namespace my
