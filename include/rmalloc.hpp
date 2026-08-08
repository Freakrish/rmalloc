#pragma once
#include "thread_cache.hpp"
#include <new>

// Public API for rmalloc.
//
// operator new/delete are defined in src/rmalloc.cpp.
// Include this header to get the declarations; link rmalloc.cpp to activate them.
//
// Memory layout of every small allocation:
//
//   raw ptr (from ThreadCache)
//   |
//   [ size_t n | ---- user data (n bytes) ---- ]
//               |
//               returned to caller
//
// The 8-byte header lets operator delete(void*) know the original size
// without a PageMap. Large objects (>256KB) go straight to PageHeap.

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
    void* raw = static_cast<char*>(p) - kHdrSize;
    ThreadCache::GetCache()->Deallocate(raw, n + kHdrSize);
}