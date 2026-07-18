#pragma once
#include "free_list.hpp"
#include "size_class.hpp"
#include "central_stub.hpp"

// Per-thread allocation cache. Each thread holds one FreeList per size class
// so that small allocations never contend on a lock.
//
// Hot path  (cache hit)  : O(1) pop/push, no lock.
// Cold path (cache miss) : batch-fetch BATCH_SIZE slots from central.
// Overflow               : return half the list back to central.
// Large objects (>256KB) : bypass the cache, go straight to central.
//
//   Thread A          Thread B
//   [ThreadCache]     [ThreadCache]   <- independent, no sharing
//        |                 |
//        +--------+--------+
//                 |
//           [central::]               <- shared (currently malloc)

class ThreadCache {
public:
    static constexpr size_t BATCH_SIZE = 32;

    void* Allocate(size_t bytes) noexcept;
    void  Deallocate(void* ptr, size_t bytes) noexcept;

    // Returns this thread's cache instance (thread_local singleton).
    static ThreadCache* GetCache() noexcept;

private:
    FreeList lists_[SizeClass::NUM_CLASSES];

    void* FetchFromCentral(size_t cl) noexcept;
    void  ReturnToCentral(size_t cl)  noexcept;
};

inline ThreadCache* ThreadCache::GetCache() noexcept {
    thread_local ThreadCache cache;
    return &cache;
}

inline void* ThreadCache::Allocate(size_t bytes) noexcept {
    if (bytes == 0) bytes = 1;
    if (bytes > SizeClass::MAX_SIZE)
        return central::allocate(bytes);

    size_t    cl   = kSizeClass.size_class(bytes);
    FreeList& list = lists_[cl];

    if (!list.empty())
        return list.pop();

    return FetchFromCentral(cl);
}

inline void ThreadCache::Deallocate(void* ptr, size_t bytes) noexcept {
    if (!ptr) return;
    if (bytes > SizeClass::MAX_SIZE) {
        central::deallocate(ptr, bytes);
        return;
    }

    size_t    cl   = kSizeClass.size_class(bytes);
    FreeList& list = lists_[cl];
    list.push(ptr);

    if (list.length() > FreeList::MAX_LENGTH)
        ReturnToCentral(cl);
}

inline void* ThreadCache::FetchFromCentral(size_t cl) noexcept {
    const size_t slot = kSizeClass.class_size(cl);
    for (size_t i = 0; i < BATCH_SIZE; ++i) {
        void* obj = central::allocate(slot);
        if (!obj) break;
        lists_[cl].push(obj);
    }
    return lists_[cl].pop();
}

inline void ThreadCache::ReturnToCentral(size_t cl) noexcept {
    const size_t slot   = kSizeClass.class_size(cl);
    FreeList&    list   = lists_[cl];
    const size_t target = list.length() / 2;
    for (size_t i = 0; i < target; ++i)
        central::deallocate(list.pop(), slot);
}