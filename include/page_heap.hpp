#pragma once
#include <cstddef>
#include "spinlock.hpp"

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <sys/mman.h>
#endif

// All memory in rmalloc originates here. Allocations are rounded up to
// kPageSize so addresses are always page-aligned.

class PageHeap {
public:
    static constexpr size_t kPageSize = 4096;

    PageHeap(const PageHeap&)            = delete;
    PageHeap& operator=(const PageHeap&) = delete;

    static PageHeap& Instance() noexcept {
        static PageHeap inst;
        return inst;
    }

    void*  Allocate(size_t bytes) noexcept;
    void   Free(void* ptr, size_t bytes) noexcept;
    size_t bytes_in_use() const noexcept { return bytes_in_use_; }

private:
    PageHeap() = default;

    static size_t round_to_page(size_t n) noexcept {
        return (n + kPageSize - 1) & ~(kPageSize - 1);
    }

    Spinlock lock_;
    size_t   bytes_in_use_{0};
};

inline void* PageHeap::Allocate(size_t bytes) noexcept {
    const size_t sz = round_to_page(bytes);

#ifdef _WIN32
    void* ptr = VirtualAlloc(nullptr, sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    void* ptr = mmap(nullptr, sz, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) ptr = nullptr;
#endif

    if (ptr) {
        LockGuard<Spinlock> lk(lock_);
        bytes_in_use_ += sz;
    }
    return ptr;
}

inline void PageHeap::Free(void* ptr, size_t bytes) noexcept {
    if (!ptr) return;
    const size_t sz = round_to_page(bytes);

#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, sz);
#endif

    LockGuard<Spinlock> lk(lock_);
    bytes_in_use_ -= sz;
}