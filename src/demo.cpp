#include <iostream>
#include <cassert>
#include <vector>
#include "size_class.hpp"
#include "thread_cache.hpp"

static void print_section(const char* title) {
    std::cout << "\n=== " << title << " ===\n";
}

static void demo_size_classes() {
    print_section("All 88 size classes");
    for (size_t i = 0; i < SizeClass::NUM_CLASSES; ++i)
        std::cout << "  class[" << i << "] = " << kSizeClass.class_size(i) << " bytes\n";

    print_section("Request -> size class");
    size_t reqs[] = {1, 7, 8, 9, 16, 17, 32, 33, 64, 100, 128, 129, 256, 1024, 65536, 262144};
    for (size_t req : reqs) {
        size_t sc   = kSizeClass.size_class(req);
        size_t slot = kSizeClass.class_size(sc);
        double frag = 100.0 * (slot - req) / slot;
        std::cout << "  " << req << "B -> class[" << sc << "]=" << slot << "B  frag=" << frag << "%\n";
    }
}

static void demo_thread_cache() {
    print_section("ThreadCache — alloc / free / reuse");

    ThreadCache* tc = ThreadCache::GetCache();

    void* p1 = tc->Allocate(20);
    void* p2 = tc->Allocate(20);
    assert(p1 && p2 && p1 != p2);

    *static_cast<int*>(p1) = 42;
    *static_cast<int*>(p2) = 99;
    std::cout << "  p1=" << p1 << "  val=" << *static_cast<int*>(p1) << "\n";
    std::cout << "  p2=" << p2 << "  val=" << *static_cast<int*>(p2) << "\n";

    tc->Deallocate(p1, 20);
    tc->Deallocate(p2, 20);

    void* p3 = tc->Allocate(20);
    assert(p3 == p2 && "LIFO: must reuse most-recently-freed slot");
    std::cout << "  p3=" << p3 << "  (reused p2 — cache hit)\n";
    tc->Deallocate(p3, 20);
}

static void demo_batch_fetch() {
    print_section("Batch fetch (512B slots, first use of this class)");

    ThreadCache* tc = ThreadCache::GetCache();
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i)
        ptrs.push_back(tc->Allocate(512));

    for (int i = 0; i < 10; ++i)
        std::cout << "  [" << i << "] " << ptrs[i] << "\n";

    for (void* p : ptrs)
        tc->Deallocate(p, 512);
}

int main() {
    demo_size_classes();
    demo_thread_cache();
    demo_batch_fetch();
    std::cout << "\nAll demos passed.\n";
}