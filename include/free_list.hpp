#pragma once
#include <cstddef>

class FreeList {
public:
    static constexpr size_t MAX_LENGTH = 256;

    void push(void* obj) noexcept {
        *static_cast<void**>(obj) = head_;
        head_ = obj;
        ++length_;
    }

    void* pop() noexcept {
        if (!head_) return nullptr;
        void* obj = head_;
        head_ = *static_cast<void**>(head_);
        --length_;
        return obj;
    }

    bool   empty()  const noexcept { return head_ == nullptr; }
    size_t length() const noexcept { return length_; }

private:
    void*  head_{nullptr};
    size_t length_{0};
};