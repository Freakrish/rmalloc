#pragma once
#include "free_list.hpp"
#include "size_class.hpp"
#include "central_stub.hpp"
#include "spinlock.hpp"

// Shared pool of free slots, one per size class.
// All thread caches drain into and refill from here.
// One spinlock per size class — threads of different sizes never block each other.

class CentralFreeList {
public:
    CentralFreeList(const CentralFreeList&)            = delete;
    CentralFreeList& operator=(const CentralFreeList&) = delete;

    static CentralFreeList& Instance() {
        static CentralFreeList inst;
        return inst;
    }

    size_t FetchBatch(size_t cl, FreeList& dst, size_t want) noexcept;

    void ReturnBatch(size_t cl, FreeList& src, size_t count) noexcept;

private:
    CentralFreeList() = default;

    struct Slab {
        FreeList list;
        Spinlock lock;
    };

    Slab slabs_[SizeClass::NUM_CLASSES];
};

inline size_t CentralFreeList::FetchBatch(size_t cl, FreeList& dst, size_t want) noexcept {
    Slab& s = slabs_[cl];
    LockGuard<Spinlock> lk(s.lock);

    if (s.list.empty()) {
        const size_t slot = kSizeClass.class_size(cl);
        for (size_t i = 0; i < want * 2; ++i) {
            void* obj = central::allocate(slot);
            if (!obj) break;
            s.list.push(obj);
        }
    }

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