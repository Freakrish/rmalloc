#pragma once
#include <cstddef>
#include <cstdint>

class SizeClass {
public:
    static constexpr size_t NUM_CLASSES = 88;
    static constexpr size_t MAX_SIZE    = 256 * 1024;

    constexpr SizeClass() {
        size_t idx = 0;

        struct Band { size_t base, step, limit; };
        constexpr Band bands[] = {
            {      0,     8,    128 },
            {    128,    16,    256 },
            {    256,    32,    512 },
            {    512,    64,   1024 },
            {   1024,   128,   2048 },
            {   2048,   256,   4096 },
            {   4096,   512,   8192 },
            {   8192,  1024,  16384 },
            {  16384,  2048,  32768 },
            {  32768,  4096,  65536 },
            {  65536,  8192, 131072 },
            { 131072, 16384, 262144 },
        };

        for (auto& b : bands)
            for (size_t s = b.base + b.step; s <= b.limit && idx < NUM_CLASSES; s += b.step)
                class_to_size_[idx++] = s;

        while (idx < NUM_CLASSES)
            class_to_size_[idx++] = MAX_SIZE;

        size_t cl = 0;
        for (size_t bytes = 0; bytes <= MAX_SIZE; bytes += 8) {
            while (cl < NUM_CLASSES - 1 && class_to_size_[cl] < bytes) ++cl;
            size_to_class_[(bytes == 0 ? 0 : (bytes - 1) / 8)] = static_cast<uint8_t>(cl);
        }
    }

    constexpr size_t class_size(size_t sc) const noexcept {
        return class_to_size_[sc];
    }

    constexpr size_t batch_size(size_t sc) const noexcept {
        size_t sz = class_to_size_[sc];
        if (sz == 0) return 1;
        size_t n = 4096 / sz;
        if (n < 1)  return 1;
        if (n > 32) return 32;
        return n;
    }

    constexpr size_t size_class(size_t bytes) const noexcept {
        if (bytes == 0) bytes = 1;
        size_t rounded = (bytes + 7) & ~size_t(7);
        if (rounded > MAX_SIZE) return NUM_CLASSES - 1;
        return size_to_class_[(rounded - 1) / 8];
    }

private:
    size_t  class_to_size_[NUM_CLASSES]{};
    uint8_t size_to_class_[MAX_SIZE / 8 + 1]{};
};

constexpr SizeClass kSizeClass;