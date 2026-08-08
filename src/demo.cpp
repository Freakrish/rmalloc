#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "rmalloc.hpp"
#include "page_heap.hpp"

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

    PageHeap& ph  = PageHeap::Instance();
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

int main() {
    demo_scalar_new_delete();
    demo_array_new_delete();
    demo_stl_containers();
    demo_page_heap_tracking();
    demo_nothrow();
    std::cout << "\nAll demos passed.\n";
}