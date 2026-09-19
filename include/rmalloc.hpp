#pragma once
#include "thread_cache.hpp"
#include <new>

static const size_t HDR = sizeof(size_t);

inline void* rmalloc(size_t n) noexcept {
    void* raw = ThreadCache::GetCache()->Allocate(n + HDR);
    if (!raw) return nullptr;
    *static_cast<size_t*>(raw) = n;
    return static_cast<char*>(raw) + HDR;
}

inline void rmfree(void* p) noexcept {
    if (!p) return;
    void*  raw = static_cast<char*>(p) - HDR;
    size_t n   = *static_cast<size_t*>(raw);
    ThreadCache::GetCache()->Deallocate(raw, n + HDR);
}

inline void rmfree(void* p, size_t n) noexcept {
    if (!p) return;
    ThreadCache::GetCache()->Deallocate(static_cast<char*>(p) - HDR, n + HDR);
}