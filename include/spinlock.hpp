#pragma once
#include <atomic>

// Spinlock built on std::atomic_flag — the only type the standard guarantees
// is truly lock-free. Suitable for short critical sections like a list pop/push.
// For long sections, prefer std::mutex (sleeps the thread instead of spinning).

class Spinlock {
public:
    void lock()   noexcept { while (flag_.test_and_set(std::memory_order_acquire)) {} }
    void unlock() noexcept { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

// RAII lock guard — acquires on construction, releases on destruction.
// Works with any type that has lock() / unlock(), including Spinlock.
// This is exactly what std::lock_guard does in the standard library.
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