#pragma once
#include <stdint.h>
using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;
using usize = size_t;

using f32 = float;
using f64 = double;

using c8 = char8_t;
using c16 = char16_t;
using c32 = char32_t;
using b8 = bool;

constexpr u32 U32_MAX = UINT32_MAX;

struct Entity {
    explicit constexpr Entity(u32 index) : index(index) { }
    constexpr Entity() : index(U32_MAX) { }
    static const Entity NONE;

    operator u32() const { return index; } // NOLINT(*-explicit-constructor)

    b8 operator!=(const Entity other) const { return index != other.index; }
    Entity& operator++() {
        index++;
        return *this;
    }
    Entity operator*() const { return *this; }

protected:
    u32 index;
};

inline const Entity Entity::NONE = Entity { };
