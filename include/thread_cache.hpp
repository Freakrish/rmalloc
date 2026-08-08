#pragma once
#include "free_list.hpp"
#include "size_class.hpp"
#include "central_free_list.hpp"
#include "page_heap.hpp"

// Per-thread allocation cache. Each thread holds one FreeList per size class
// so that small allocations never contend on a lock.
//
// Hot path  (cache hit)  : O(1) pop/push, no lock.
// Cold path (cache miss) : batch-fetch BATCH_SIZE slots from CentralFreeList.
// Overflow               : return half the list back to CentralFreeList.
// Large objects (>256KB) : skip the cache, go straight to PageHeap.

class ThreadCache {
public:
    static constexpr size_t BATCH_SIZE = 32;

    void* Allocate(size_t bytes) noexcept;
    void  Deallocate(void* ptr, size_t bytes) noexcept;

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
        return PageHeap::Instance().Allocate(bytes);

    size_t    cl   = kSizeClass.size_class(bytes);
    FreeList& list = lists_[cl];

    if (!list.empty())
        return list.pop();

    return FetchFromCentral(cl);
}

inline void ThreadCache::Deallocate(void* ptr, size_t bytes) noexcept {
    if (!ptr) return;
    if (bytes > SizeClass::MAX_SIZE) {
        PageHeap::Instance().Free(ptr, bytes);
        return;
    }

    size_t    cl   = kSizeClass.size_class(bytes);
    FreeList& list = lists_[cl];
    list.push(ptr);

    if (list.length() > FreeList::MAX_LENGTH)
        ReturnToCentral(cl);
}

inline void* ThreadCache::FetchFromCentral(size_t cl) noexcept {
    CentralFreeList::Instance().FetchBatch(cl, lists_[cl], BATCH_SIZE);
    return lists_[cl].pop();
}

inline void ThreadCache::ReturnToCentral(size_t cl) noexcept {
    const size_t count = lists_[cl].length() / 2;
    CentralFreeList::Instance().ReturnBatch(cl, lists_[cl], count);
}