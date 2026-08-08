#include "rmalloc.hpp"

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

void* operator new(size_t n, std::nothrow_t const&)  noexcept { return rmalloc(n); }
void* operator new[](size_t n, std::nothrow_t const&) noexcept { return rmalloc(n); }

void operator delete(void* p)              noexcept { rmfree(p); }
void operator delete[](void* p)            noexcept { rmfree(p); }
void operator delete(void* p, size_t n)    noexcept { rmfree(p, n); }
void operator delete[](void* p, size_t n)  noexcept { rmfree(p, n); }