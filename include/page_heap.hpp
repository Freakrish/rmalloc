#pragma once
#include <cstddef>
#include "spinlock.hpp"

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <sys/mman.h>
#endif

// Talks directly to the OS. All memory in rmalloc originates here.
//
// Allocate() asks the OS for committed, zeroed pages.
// Free()     returns them — the OS reclaims the physical frames immediately.
//
// Every allocation is rounded up to kPageSize (4 KB) so addresses are always
// page-aligned. CentralFreeList carves pages into same-sized slots;
// large objects (> 256 KB) get their own span and skip CentralFreeList.

class PageHeap {
public:
    static constexpr size_t kPageSize = 4096;

    PageHeap(const PageHeap&)            = delete;
    PageHeap& operator=(const PageHeap&) = delete;

    static PageHeap& Instance() noexcept {
        static PageHeap inst;
        return inst;
    }

    // Returns page-aligned memory, or nullptr on OOM.
    void* Allocate(size_t bytes) noexcept;

    // bytes must equal what was passed to Allocate.
    void  Free(void* ptr, size_t bytes) noexcept;

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