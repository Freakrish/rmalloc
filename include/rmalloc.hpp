#pragma once
#include "thread_cache.hpp"
#include <new>

static constexpr size_t kHdrSize = sizeof(size_t);

inline void* rmalloc(size_t n) noexcept {
    void* raw = ThreadCache::GetCache()->Allocate(n + kHdrSize);
    if (!raw) return nullptr;
    *static_cast<size_t*>(raw) = n;
    return static_cast<char*>(raw) + kHdrSize;
}

inline void rmfree(void* p) noexcept {
    if (!p) return;
    void*  raw = static_cast<char*>(p) - kHdrSize;
    size_t n   = *static_cast<size_t*>(raw);
    ThreadCache::GetCache()->Deallocate(raw, n + kHdrSize);
}

inline void rmfree(void* p, size_t n) noexcept {
    if (!p) return;
    ThreadCache::GetCache()->Deallocate(static_cast<char*>(p) - kHdrSize, n + kHdrSize);
}