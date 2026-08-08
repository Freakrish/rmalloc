#pragma once
#include <atomic>

class Spinlock {
public:
    void lock()   noexcept { while (flag_.test_and_set(std::memory_order_acquire)) {} }
    void unlock() noexcept { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

// RAII wrapper — mirrors std::lock_guard, works with any lock() / unlock() type.
template<typename Lock>
class LockGuard {
public:
    explicit LockGuard(Lock& l) noexcept : lock_(l) { lock_.lock(); }
    ~LockGuard()                noexcept             { lock_.unlock(); }

    LockGuard(const LockGuard&)            = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    Lock& lock_;
};