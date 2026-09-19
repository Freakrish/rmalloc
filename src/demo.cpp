#include <iostream>
#include <iomanip>
#include <cassert>
#include <vector>
#include <string>
#include "rmalloc.hpp"
#include <process.h>
#include "page_heap.hpp"
#include "central_free_list.hpp"

struct Point {
    double x, y, z;
    Point(double x, double y, double z) : x(x), y(y), z(z) {}
};

static void test_basics() {
    puts("\n--- new / delete ---");

    int* n = new int(42);
    printf("  int:   %d  @ %p\n", *n, n);
    delete n;

    Point* p = new Point(1, 2, 3);
    printf("  Point: (%.0f, %.0f, %.0f)  @ %p\n", p->x, p->y, p->z, p);
    delete p;

    int* arr = new int[10];
    for (int i = 0; i < 10; ++i) arr[i] = i * i;
    printf("  arr[9] = %d\n", arr[9]);
    delete[] arr;
}

static void test_stl() {
    puts("\n--- STL ---");

    std::vector<int> v;
    for (int i = 0; i < 100; ++i) v.push_back(i);
    std::cout << "  vector size=" << v.size() << " back=" << v.back() << "\n";

    std::string s = "rmalloc owns this";
    std::cout << "  string: " << s << "\n";
}

static void test_pageheap() {
    puts("\n--- PageHeap tracking ---");

    PageHeap& ph = PageHeap::Instance();
    size_t before = ph.bytes_in_use();

    int*    a = new int(1);
    double* b = new double[100];
    Point*  c = new Point(0, 0, 0);

    printf("  delta = %zuB\n", ph.bytes_in_use() - before);

    delete a; delete[] b; delete c;
}

static void test_nothrow() {
    puts("\n--- nothrow new ---");

    void* p = operator new(16, std::nothrow);
    assert(p);
    printf("  got %p\n", p);
    operator delete(p);
}

static unsigned __stdcall worker(void*) {
    ThreadCache* tc = ThreadCache::GetCache();
    for (int i = 0; i < 50; ++i)
        tc->Allocate(64);
    return 0;
}

static void test_flush() {
    puts("\n--- thread cache flush ---");

    size_t cl = kSizeClass.size_class(64);

    HANDLE h = (HANDLE)_beginthreadex(nullptr, 0, worker, nullptr, 0, nullptr);
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);

    FreeList got;
    CentralFreeList::Instance().FetchBatch(cl, got, 50);
    printf("  recovered %zu slots\n", got.length());
    CentralFreeList::Instance().ReturnBatch(cl, got, got.length());
}

static void test_stats() {
    puts("\n--- central free list stats ---");

    CentralFreeList& cfl = CentralFreeList::Instance();
    printf("  %-6s %-8s %-8s %-8s %-8s\n", "class", "slot", "batch", "refills", "cached");

    for (size_t cl = 0; cl < SizeClass::NUM_CLASSES; ++cl) {
        auto s = cfl.stats(cl);
        if (s.refills == 0) continue;
        printf("  %-6zu %-8zu %-8zu %-8zu %-8zu\n",
               cl, kSizeClass.class_size(cl), kSizeClass.batch_size(cl),
               s.refills, s.cached);
    }
}

int main() {
    test_basics();
    test_stl();
    test_pageheap();
    test_nothrow();
    test_flush();
    test_stats();
}