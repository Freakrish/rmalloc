#pragma once
#include "free_list.hpp"
#include "size_class.hpp"
#include "spinlock.hpp"
#include "page_heap.hpp"

// Shared pool of free slots, one per size class.
// On underflow, carves a fresh OS page into same-sized slots.
// One spinlock per size class to minimize contention between threads.

class CentralFreeList {
public:
    CentralFreeList(const CentralFreeList&)            = delete;
    CentralFreeList& operator=(const CentralFreeList&) = delete;

    static CentralFreeList& Instance() {
        static CentralFreeList inst;
        return inst;
    }

    struct SlabStats {
        size_t refills;   // times this class ran dry and needed a new page
        size_t cached;    // slots currently sitting in the central slab
    };

    size_t   FetchBatch(size_t cl, FreeList& dst, size_t want) noexcept;
    void     ReturnBatch(size_t cl, FreeList& src, size_t count) noexcept;
    SlabStats stats(size_t cl) const noexcept;

private:
    CentralFreeList() = default;

    void Refill(size_t cl) noexcept;

    struct Slab {
        FreeList list;
        Spinlock lock;
        size_t   refill_count{0};
    };

    Slab slabs_[SizeClass::NUM_CLASSES];
};

inline void CentralFreeList::Refill(size_t cl) noexcept {
    const size_t slot_size  = kSizeClass.class_size(cl);
    const size_t alloc_size = slot_size > PageHeap::kPageSize ? slot_size : PageHeap::kPageSize;

    char* span = static_cast<char*>(PageHeap::Instance().Allocate(alloc_size));
    if (!span) return;

    ++slabs_[cl].refill_count;
    const size_t n_slots = alloc_size / slot_size;
    for (size_t i = 0; i < n_slots; ++i)
        slabs_[cl].list.push(span + i * slot_size);
}

inline CentralFreeList::SlabStats CentralFreeList::stats(size_t cl) const noexcept {
    Slab& s = const_cast<Slab&>(slabs_[cl]);
    LockGuard<Spinlock> lk(s.lock);
    return { s.refill_count, s.list.length() };
}

inline size_t CentralFreeList::FetchBatch(size_t cl, FreeList& dst, size_t want) noexcept {
    Slab& s = slabs_[cl];
    LockGuard<Spinlock> lk(s.lock);

    if (s.list.empty())
        Refill(cl);

    size_t moved = 0;
    while (moved < want && !s.list.empty()) {
        dst.push(s.list.pop());
        ++moved;
    }
    return moved;
}

inline void CentralFreeList::ReturnBatch(size_t cl, FreeList& src, size_t count) noexcept {
    Slab& s = slabs_[cl];
    LockGuard<Spinlock> lk(s.lock);
    for (size_t i = 0; i < count && !src.empty(); ++i)
        s.list.push(src.pop());
}