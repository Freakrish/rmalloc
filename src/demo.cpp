#include <iostream>
#include <iomanip>
#include <cassert>
#include <vector>
#include <string>
#include "rmalloc.hpp"
#include <process.h>
#include "page_heap.hpp"
#include "central_free_list.hpp"

static void print_section(const char* title) {
    std::cout << "\n=== " << title << " ===\n";
}

struct Point {
    double x, y, z;
    Point(double x, double y, double z) : x(x), y(y), z(z) {}
};

static void demo_scalar_new_delete() {
    print_section("scalar new / delete");

    int* n = new int(42);
    std::cout << "  new int(42)  = " << *n << "  @ " << n << "\n";
    delete n;

    Point* p = new Point(1.0, 2.0, 3.0);
    std::cout << "  new Point    = (" << p->x << ", " << p->y << ", " << p->z << ")  @ " << p << "\n";
    delete p;
}

static void demo_array_new_delete() {
    print_section("array new[] / delete[]");

    int* arr = new int[10];
    for (int i = 0; i < 10; ++i) arr[i] = i * i;
    std::cout << "  arr[9] = " << arr[9] << "\n";
    delete[] arr;

    double* buf = new double[64];
    buf[0] = 3.14;
    std::cout << "  buf[0] = " << buf[0] << "\n";
    delete[] buf;
}

static void demo_stl_containers() {
    print_section("STL containers (all use operator new internally)");

    std::vector<int> v;
    for (int i = 0; i < 100; ++i) v.push_back(i);
    std::cout << "  vector<int> size=" << v.size() << "  back=" << v.back() << "\n";

    std::string s = "rmalloc is handling this string's heap allocation";
    std::cout << "  string: \"" << s << "\"\n";
}

static void demo_page_heap_tracking() {
    print_section("PageHeap — bytes_in_use tracking");

    PageHeap& ph    = PageHeap::Instance();
    size_t    before = ph.bytes_in_use();

    int*    a = new int(1);
    double* b = new double[100];
    Point*  c = new Point(0, 0, 0);

    std::cout << "  3 allocations done\n";
    std::cout << "  PageHeap delta = " << ph.bytes_in_use() - before << "B\n";

    delete a;
    delete[] b;
    delete c;
}

static void demo_nothrow() {
    print_section("nothrow new — returns nullptr on failure");

    void* p = operator new(16, std::nothrow);
    assert(p);
    std::cout << "  nothrow new(16) @ " << p << "\n";
    operator delete(p);
}

// Worker allocates 50 slots into its ThreadCache then exits.
// The FLS destructor (tc_fls_destroy) calls Flush(), returning
// whatever slots remain in the cache to CentralFreeList.
static unsigned __stdcall leak_worker(void*) {
    ThreadCache* tc = ThreadCache::GetCache();
    for (int i = 0; i < 50; ++i)
        tc->Allocate(64);
    return 0;
}

static void demo_thread_cache_flush() {
    print_section("ThreadCache flush on thread exit (FLS destructor)");

    constexpr size_t SZ = 64;
    const size_t     cl = kSizeClass.size_class(SZ);

    HANDLE h = (HANDLE)_beginthreadex(nullptr, 0, leak_worker, nullptr, 0, nullptr);
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);

    FreeList recovered;
    CentralFreeList::Instance().FetchBatch(cl, recovered, 50);
    std::cout << "  thread held 50 allocs; FLS destructor recovered "
              << recovered.length() << " slots to CentralFreeList\n";
    CentralFreeList::Instance().ReturnBatch(cl, recovered, recovered.length());
}

static void demo_central_stats() {
    print_section("CentralFreeList per-class stats");

    CentralFreeList& cfl = CentralFreeList::Instance();
    std::cout << "  " << std::left
              << std::setw(6)  << "class"
              << std::setw(10) << "slot(B)"
              << std::setw(10) << "refills"
              << std::setw(10) << "cached"
              << "\n";

    for (size_t cl = 0; cl < SizeClass::NUM_CLASSES; ++cl) {
        auto s = cfl.stats(cl);
        if (s.refills == 0) continue;
        std::cout << "  "
                  << std::setw(6)  << cl
                  << std::setw(10) << kSizeClass.class_size(cl)
                  << std::setw(10) << s.refills
                  << std::setw(10) << s.cached
                  << "\n";
    }
}

int main() {
    demo_scalar_new_delete();
    demo_array_new_delete();
    demo_stl_containers();
    demo_page_heap_tracking();
    demo_nothrow();
    demo_thread_cache_flush();
    demo_central_stats();
    std::cout << "\nAll demos passed.\n";
}