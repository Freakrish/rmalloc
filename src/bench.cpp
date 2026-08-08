#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include "rmalloc.hpp"

using Clock = std::chrono::high_resolution_clock;
using Ms    = std::chrono::milliseconds;
using Ns    = std::chrono::nanoseconds;

using AllocFn = void*(*)(size_t);
using FreeFn  = void (*)(void*);

static volatile uint8_t sink = 0;

static void* rm_alloc (size_t n) { return rmalloc(n); }
static void  rm_free  (void*  p) { rmfree(p); }
static void* sys_alloc(size_t n) { return std::malloc(n); }
static void  sys_free (void*  p) { std::free(p); }

// Run alloc+free until at least min_ms milliseconds have elapsed.
// Returns average ns per alloc+free pair.
static double bench_latency(AllocFn alloc, FreeFn fre, size_t size, int min_ms = 200) {
    // warmup
    for (int i = 0; i < 512; ++i) {
        void* p = alloc(size);
        if (p) { sink ^= *static_cast<uint8_t*>(p); fre(p); }
    }

    long long iters = 0;
    auto t0      = Clock::now();
    auto deadline = t0 + Ms(min_ms);

    while (Clock::now() < deadline) {
        void* p = alloc(size);
        if (!p) break;
        sink ^= *static_cast<uint8_t*>(p);
        fre(p);
        ++iters;
    }

    auto t1 = Clock::now();
    if (iters == 0) return -1.0;
    double total_ns = static_cast<double>(std::chrono::duration_cast<Ns>(t1 - t0).count());
    return total_ns / iters;
}

// Batch alloc N objects, then free them all.
static double bench_throughput(AllocFn alloc, FreeFn fre, size_t size, int n) {
    std::vector<void*> ptrs(n, nullptr);

    for (int i = 0; i < 64; ++i) { void* p = alloc(size); if (p) fre(p); }

    auto t0  = Clock::now();
    int live = 0;

    for (int i = 0; i < n; ++i) {
        ptrs[i] = alloc(size);
        if (!ptrs[i]) break;
        ++live;
    }
    for (int i = 0; i < live; ++i) {
        sink ^= *static_cast<uint8_t*>(ptrs[i]);
        fre(ptrs[i]);
    }

    auto t1 = Clock::now();
    if (live == 0) return -1.0;
    double ns = static_cast<double>(std::chrono::duration_cast<Ns>(t1 - t0).count());
    return ns / (2.0 * live);
}

static void header() {
    std::cout << "\n"
              << std::left  << std::setw(10) << "size"
              << std::right << std::setw(13) << "rmalloc"
              << std::right << std::setw(13) << "malloc"
              << std::right << std::setw(10) << "speedup"
              << "\n" << std::string(46, '-') << "\n";
}

static void row(size_t size, double rm, double sys) {
    if (rm <= 0 || sys <= 0) {
        std::cout << std::left << std::setw(10) << size << "  (measurement failed)\n";
        return;
    }
    std::cout << std::left  << std::setw(10) << size
              << std::right << std::setw(11) << std::fixed << std::setprecision(1) << rm  << "ns"
              << std::right << std::setw(11) << std::fixed << std::setprecision(1) << sys << "ns"
              << std::right << std::setw(8)  << std::fixed << std::setprecision(2) << sys / rm << "x"
              << "\n";
}

int main() {
    size_t sizes[]   = {8, 16, 32, 64, 128, 256, 512, 1024};
    const int T_N    = 50000;

    std::cout << "=== Latency  (alloc+free per op, ~200ms per size) ===";
    header();
    for (size_t sz : sizes) {
        double rm  = bench_latency(rm_alloc,  rm_free,  sz);
        double sys = bench_latency(sys_alloc, sys_free, sz);
        row(sz, rm, sys);
    }

    std::cout << "\n=== Throughput  (batch alloc then batch free, " << T_N/1000 << "k ops) ===";
    header();
    for (size_t sz : sizes) {
        double rm  = bench_throughput(rm_alloc,  rm_free,  sz, T_N);
        double sys = bench_throughput(sys_alloc, sys_free, sz, T_N);
        row(sz, rm, sys);
    }

    std::cout << "\n(sink=" << static_cast<int>(sink) << ")\n";
}