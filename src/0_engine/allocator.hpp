#pragma once

#include <ranges>
#include <vector>

#include "0_engine/u_types.hpp"

namespace pce::allocator {
// Big allocation alignment should at least match vector register alignment and be power of 2
inline constexpr size_t BIG_ALLOCATION_THRESHOLD = 4096; // 4kb = page size
inline constexpr size_t BIG_ALLOCATION_ALIGNMENT = 32;   // 256-bit SIMD operations

#ifdef DEBUG
inline constexpr size_t NON_USER_SIZE = 2U * sizeof(void*) + BIG_ALLOCATION_ALIGNMENT - 1U;
#else // !DEBUG
inline constexpr size_t NON_USER_SIZE = sizeof(void*) + BIG_ALLOCATION_ALIGNMENT - 1U;
#endif // END !DEBUG
#ifdef WIN64
inline constexpr size_t BIG_ALLOCATION_SENTINEL = 0xFAFAFAFAFAFAFAFAULL;
#else // !WIN64
inline constexpr size_t BIG_ALLOCATION_SENTINEL = 0xFAFAFAFAUL;
#endif // end !WIN64

template <size_t Size> [[nodiscard]] constexpr size_t GetBlock(const size_t count) {
    if constexpr (Size > 1) { // this wouldn't be needed in a 64-bit system. I feel like this can be removed. or maybe made a debug assert. WHY would the programmer ask for so much memory?
        if (count > static_cast<size_t>(-1) / Size) { throw std::bad_array_new_length { }; }
    }
    return count * Size;
}

template <class Traits> __declspec(allocator) void *AllocateManuallyVectorAligned(const size_t bytes) { // allocate _Bytes manually aligned to at least _Big_allocation_alignment
    const size_t block_size = NON_USER_SIZE + bytes;
    if (block_size <= bytes) { throw std::bad_array_new_length { }; } // overflow?? how is this possible with 64-bit systems
    const uintptr_t ptr_container = reinterpret_cast<uintptr_t>(Traits::Allocate(block_size));
    STL_VERIFY(ptr_container != 0, "invalid argument"); // validate even in release since we're doing p[-1]
    void* const ptr = reinterpret_cast<void*>(ptr_container + NON_USER_SIZE & ~(BIG_ALLOCATION_ALIGNMENT - 1)); // masked so it starts at least at BIG_ALINGMENT
    static_cast<uintptr_t*>(ptr)[-1] = ptr_container;
    #ifdef DEBUG
    static_cast<uintptr_t*>(ptr)[-2] = BIG_ALLOCATION_SENTINEL;
    #endif // defined(DEBUG)
    return ptr;
}
inline void AdjustManuallyVectorAligned(void*& ptr, size_t& bytes) { // adjust parameters from _Allocate_manually_vector_aligned to pass to operator delete
    bytes += NON_USER_SIZE;
    const uintptr_t* const ptr_user = static_cast<uintptr_t*>(ptr);
    const uintptr_t ptr_container = ptr_user[-1];
    STL_ASSERT(ptr_user[-2] == BIG_ALLOCATION_SENTINEL, "invalid argument"); // If the following asserts, it likely means that we are performing. an aligned delete on memory coming from an unaligned allocation.
    #ifdef DEBUG
    constexpr uintptr_t min_back_shift = 2 * sizeof(void*);
    #else // ^^^ defined(DEBUG) / !defined(DEBUG) vvv
    constexpr uintptr_t min_back_shift = sizeof(void*);
    #endif // ^^^ !defined(DEBUG) ^^^
    const uintptr_t back_shift = reinterpret_cast<uintptr_t>(ptr) - ptr_container;
    STL_VERIFY(back_shift >= min_back_shift && back_shift <= NON_USER_SIZE, "invalid argument"); // Extra paranoia on aligned allocation/deallocation; ensure _Ptr_container is in range [_Min_back_shift, _Non_user_size]
    ptr = reinterpret_cast<void*>(ptr_container);
}

template <size_t Alignment> constexpr void Deallocate(void* ptr, size_t bytes) noexcept {
    if consteval {
        operator delete(ptr);
        return;
    }
    if constexpr (Alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
        size_t alignment = Alignment;
        #ifdef SIMD_OPTIMIZE
        if (bytes >= BIG_ALLOCATION_THRESHOLD) { alignment = std::max(Alignment, BIG_ALLOCATION_ALIGNMENT); } // boost the alignment of big allocations to help autovectorization
        #endif // SIMD_OPTIMIZE
        operator delete(ptr, bytes, std::align_val_t { alignment });
    } else {
        #ifdef SIMD_OPTIMIZE
        if (bytes >= BIG_ALLOCATION_THRESHOLD) { AdjustManuallyVectorAligned(ptr, bytes); } // boost the alignment of big allocations to help autovectorization
        #endif // SIMD_OPTIMIZE
        operator delete(ptr, bytes);
    }
}

struct DefaultAllocateTraits {
    __declspec(allocator) static void *Allocate(const size_t bytes) { return operator new(bytes); }
    __declspec(allocator) static constexpr void *AllocateAligned(const size_t bytes, const size_t align) { return operator new(bytes, std::align_val_t { align }); }
};

template <size_t Alignment, class Traits = DefaultAllocateTraits> __declspec(allocator) constexpr void *Allocate(const size_t bytes) {
    if (bytes == 0) { return nullptr; }
    if consteval { return Traits::Allocate(bytes); }
    if constexpr (Alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
        size_t alignment = Alignment;
        #ifdef SIMD_OPTIMIZE
        if (bytes >= BIG_ALLOCATION_THRESHOLD) { alignment = std::max(Alignment, BIG_ALLOCATION_ALIGNMENT); } // boost the alignment of big allocations to help autovectorization
        #endif // SIMD_OPTIMIZE
        return Traits::AllocateAligned(bytes, alignment);
    } else {
        #ifdef SIMD_OPTIMIZE
        if (bytes >= BIG_ALLOCATION_THRESHOLD) { return AllocateManuallyVectorAligned<Traits>(bytes); } // boost the alignment of big allocations to help autovectorization
        #endif // SIMD_OPTIMIZE
        return Traits::Allocate(bytes);
    }
}

template <class T> class Allocator {
public:
    static_assert(!std::is_const_v<T>, "The C++ Standard forbids containers of const elements " "because allocator<const T> is ill-formed.");
    static_assert(!std::is_function_v<T>, "The C++ Standard forbids allocators for function elements " "because of [allocator.requirements].");
    static_assert(!std::is_reference_v<T>, "The C++ Standard forbids allocators for reference elements " "because of [allocator.requirements].");

    using _From_primary = Allocator;
    using value_type = T;
    using size_type = u32;
    using difference_type = i32;
    using propagate_on_container_move_assignment = std::true_type;

    constexpr Allocator() noexcept { }
    constexpr Allocator(const Allocator&) noexcept = default;
    template <class _Other> constexpr Allocator(const Allocator<_Other>&) noexcept { }
    constexpr ~Allocator() = default;
    constexpr Allocator& operator=(const Allocator&) = default;

    static constexpr void deallocate(T* const ptr, const u32 count) noexcept {
        _STL_ASSERT(ptr != nullptr || count == 0, "null pointer cannot point to a block of non-zero size");
        const i32 alignment = std::max(alignof(T), __STDCPP_DEFAULT_NEW_ALIGNMENT__);
        const size_t block = sizeof(T) * count;
        Deallocate<alignment>(ptr, block);
    }
    [[nodiscard]] static constexpr __declspec(allocator) T *allocate(const u32 count) {
        static_assert(sizeof(value_type) > 0, "value_type must be complete before calling allocate.");
        const i32 alignment = std::max(alignof(T), __STDCPP_DEFAULT_NEW_ALIGNMENT__);
        const size_t block = GetBlock<sizeof(T)>(count);
        return static_cast<T*>(Allocate<alignment>(block));
    }
    [[nodiscard]] constexpr std::allocation_result<T*> allocate_at_least(const u32 count) { return { allocate(count), count }; }
};


inline void TestingEnumerate () {
    std::vector<double, Allocator<double>> v;
    v.push_back(1.0); v.push_back(1.0);
    v.push_back(1.0);
    v.push_back(1.0);
    v.push_back(1.0);
    for (auto [i, d] : v | std::views::enumerate) {
    }
}
} // namespace pce::allocator
