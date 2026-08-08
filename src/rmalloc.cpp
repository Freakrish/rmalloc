#include "rmalloc.hpp"

// Global operator new / delete overrides.
//
// Defining these in ONE .cpp file replaces the runtime's default allocator
// for the entire program. Every new/delete call in every translation unit
// routes here — no call sites need to change.
//
// We provide four pairs:
//   scalar new/delete           — for  `new T`   / `delete p`
//   array  new[]/delete[]       — for  `new T[]` / `delete[] p`
//   nothrow variants            — for  `new (std::nothrow) T` callers
//   sized delete (C++14)        — compiler prefers this over unsized delete

void* operator new(size_t n) {
    void* p = rmalloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}

void* operator new[](size_t n) {
    void* p = rmalloc(n);
    if (!p) throw std::bad_alloc();
    return p;
}

void* operator new(size_t n, std::nothrow_t const&) noexcept  { return rmalloc(n); }
void* operator new[](size_t n, std::nothrow_t const&) noexcept { return rmalloc(n); }

void operator delete(void* p)   noexcept             { rmfree(p); }
void operator delete[](void* p) noexcept             { rmfree(p); }

// Sized delete: compiler passes the original size when it knows it.
// Preferred over the unsized form — skips reading the header.
void operator delete(void* p, size_t n)   noexcept   { rmfree(p, n); }
void operator delete[](void* p, size_t n) noexcept   { rmfree(p, n); }