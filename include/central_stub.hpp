#pragma once
#include <cstdlib>
#include <cstddef>

// Temporary stand-in for CentralFreeList — passes through to malloc/free.
namespace central {

inline void* allocate(size_t bytes) noexcept {
    return std::malloc(bytes);
}

inline void deallocate(void* ptr, size_t) noexcept {
    std::free(ptr);
}

}
