#pragma once
#include "free_list.hpp"
#include "size_class.hpp"
#include "central_free_list.hpp"
#include "page_heap.hpp"

class ThreadCache {
public:

    void  Flush() noexcept;
    void* Allocate(size_t bytes) noexcept;
    void  Deallocate(void* ptr, size_t bytes) noexcept;

    static ThreadCache* GetCache() noexcept;

private:
    FreeList lists_[SizeClass::NUM_CLASSES];

    void* FetchFromCentral(size_t cl) noexcept;
    void  ReturnToCentral(size_t cl)  noexcept;
};

#ifdef _WIN32

// MinGW 6 headers omit FLS; symbols live in kernel32.dll so we declare them.
#ifndef FLS_OUT_OF_INDEXES
extern "C" {
    typedef void (WINAPI *PFLS_CALLBACK_FUNCTION)(PVOID);
    DWORD WINAPI FlsAlloc(PFLS_CALLBACK_FUNCTION);
    PVOID WINAPI FlsGetValue(DWORD);
    BOOL  WINAPI FlsSetValue(DWORD, PVOID);
    BOOL  WINAPI FlsFree(DWORD);
}
#define FLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#endif

// Forward-declare so tc_fls_slot can reference it before the full definition.
inline void WINAPI tc_fls_destroy(PVOID val);

// One FLS slot shared across all TUs via the inline magic-static.
inline DWORD tc_fls_slot() noexcept {
    static DWORD s = FlsAlloc(tc_fls_destroy);
    return s;
}

inline void ThreadCache::Flush() noexcept {
    for (size_t cl = 0; cl < SizeClass::NUM_CLASSES; ++cl)
        if (!lists_[cl].empty())
            CentralFreeList::Instance().ReturnBatch(cl, lists_[cl], lists_[cl].length());
}

inline ThreadCache* ThreadCache::GetCache() noexcept {
    DWORD slot = tc_fls_slot();
    if (slot == FLS_OUT_OF_INDEXES) return nullptr;
    void* p = FlsGetValue(slot);
    if (!p) {
        p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ThreadCache));
        if (p) FlsSetValue(slot, p);
    }
    return static_cast<ThreadCache*>(p);
}

// Windows calls this when any thread exits; p is the ThreadCache for that thread.
inline void WINAPI tc_fls_destroy(PVOID p) {
    if (!p) return;
    static_cast<ThreadCache*>(p)->Flush();
    HeapFree(GetProcessHeap(), 0, p);
}

#else  // POSIX: thread_local + destructor works correctly

inline void ThreadCache::Flush() noexcept {
    for (size_t cl = 0; cl < SizeClass::NUM_CLASSES; ++cl)
        if (!lists_[cl].empty())
            CentralFreeList::Instance().ReturnBatch(cl, lists_[cl], lists_[cl].length());
}

inline ThreadCache* ThreadCache::GetCache() noexcept {
    static thread_local struct Guard {
        ThreadCache tc;
        ~Guard() { tc.Flush(); }
    } g;
    return &g.tc;
}

#endif

inline void* ThreadCache::Allocate(size_t bytes) noexcept {
    if (bytes == 0) bytes = 1;
    if (bytes > SizeClass::MAX_SIZE)
        return PageHeap::Instance().Allocate(bytes);

    size_t    cl   = kSizeClass.size_class(bytes);
    FreeList& list = lists_[cl];

    if (!list.empty())
        return list.pop();

    return FetchFromCentral(cl);
}

inline void ThreadCache::Deallocate(void* ptr, size_t bytes) noexcept {
    if (!ptr) return;
    if (bytes > SizeClass::MAX_SIZE) {
        PageHeap::Instance().Free(ptr, bytes);
        return;
    }

    size_t    cl   = kSizeClass.size_class(bytes);
    FreeList& list = lists_[cl];
    list.push(ptr);

    if (list.length() > FreeList::MAX_LENGTH)
        ReturnToCentral(cl);
}

inline void* ThreadCache::FetchFromCentral(size_t cl) noexcept {
    CentralFreeList::Instance().FetchBatch(cl, lists_[cl], kSizeClass.batch_size(cl));
    return lists_[cl].pop();
}

inline void ThreadCache::ReturnToCentral(size_t cl) noexcept {
    const size_t keep  = kSizeClass.batch_size(cl);
    const size_t count = lists_[cl].length() > keep ? lists_[cl].length() - keep : 0;
    if (count) CentralFreeList::Instance().ReturnBatch(cl, lists_[cl], count);
}