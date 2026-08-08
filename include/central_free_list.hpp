#pragma once
#include "free_list.hpp"
#include "size_class.hpp"
#include "spinlock.hpp"
#include "page_heap.hpp"

// Shared pool of free slots, one per size class.
// When a slab runs dry it asks PageHeap for a full page and carves it into slots.
// One spinlock per size class — threads of different sizes never block each other.

class CentralFreeList {
public:
    CentralFreeList(const CentralFreeList&)            = delete;
    CentralFreeList& operator=(const CentralFreeList&) = delete;

    static CentralFreeList& Instance() {
        static CentralFreeList inst;
        return inst;
    }

    // Pop up to `want` objects into dst. Returns count moved.
    size_t FetchBatch(size_t cl, FreeList& dst, size_t want) noexcept;

    // Push up to `count` objects from src into the central list for cl.
    void ReturnBatch(size_t cl, FreeList& src, size_t count) noexcept;

private:
    CentralFreeList() = default;

    // Carve one OS page into slots and push them onto the slab list.
    // Called under the slab lock.
    void Refill(size_t cl) noexcept;

    struct Slab {
        FreeList list;
        Spinlock lock;
    };

    Slab slabs_[SizeClass::NUM_CLASSES];
};

inline void CentralFreeList::Refill(size_t cl) noexcept {
    const size_t slot_size  = kSizeClass.class_size(cl);
    // Allocate at least one full page; for slots larger than a page, one slot.
    const size_t alloc_size = slot_size > PageHeap::kPageSize
                                ? slot_size
                                : PageHeap::kPageSize;

    char* span = static_cast<char*>(PageHeap::Instance().Allocate(alloc_size));
    if (!span) return;

    const size_t n_slots = alloc_size / slot_size;
    for (size_t i = 0; i < n_slots; ++i)
        slabs_[cl].list.push(span + i * slot_size);
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