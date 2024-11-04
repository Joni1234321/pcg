#pragma once
using i8 = signed char;
using u8 = unsigned char;
using i16 = signed short;
using u16 = unsigned short;
using i32 = signed int;
using u32 = unsigned int;
using i64 = signed long long;
using u64 = unsigned long long;
using usize = size_t;

using f32 = float;
using f64 = double;

using c8 = char;
using c16 = char16_t;
using c32 = char32_t;
using b8 = bool;

constexpr u32 U32_MAX = 0xFFFFFFFF;

struct Entity {
    constexpr Entity() : index(static_cast<u32>(-1)) {}
    constexpr Entity(u32 index) : index(index) {}
    operator u32() const { return index; } // NOLINT(*-explicit-constructor)

    bool operator!=(const Entity other) const { return index != other.index; }
    Entity& operator++() { index++; return *this; }
    Entity operator*() const { return *this; }
protected:
    u32 index;
};
