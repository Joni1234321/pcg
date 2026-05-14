// ReSharper disable CppInconsistentNaming
#pragma once
#include <cstdint>
#include <optional>
#include <type_traits>

#if defined(_DEBUG) || defined(NDEBUG)
    #define DEBUG
#endif // DEBUG
#if defined(_M_IX86) || defined(_M_X64)
    #define SIMD_OPTIMIZE
#endif // SIMD_OPTIMIZE
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    #ifndef WIN64
        #define WIN64
    #endif
#endif // WIN64

#if defined(_MSC_VER)
    #define STL_VERIFY(cond, mesg) _STL_VERIFY(cond, mesg)
#else
    #include <cassert>
    #define STL_VERIFY(cond, mesg) assert((cond) && (mesg))
#endif
#ifdef DEBUG
    #define STL_ASSERT(cond, mesg) STL_VERIFY(cond, mesg)
#else // DEBUG
    #if defined(_MSC_VER)
        #define STL_ASSERT(cond, mesg) _Analysis_assume_(cond)
    #else
        #define STL_ASSERT(cond, mesg) ((void)(cond))
    #endif
#endif // !DEBUG

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
struct float2;
struct uint2;

struct uint2 {
    u32 x;
    u32 y;
    constexpr uint2 operator+(const uint2 other) const { return { x + other.x, y + other.y }; }
    constexpr uint2 operator-(const uint2 other) const { return { x - other.x, y - other.y }; }
    constexpr uint2 operator*(const uint2 other) const { return { x * other.x, y * other.y }; }
    constexpr uint2 operator/(const uint2 other) const { return { x / other.x, y / other.y }; }
    constexpr uint2 operator*(const u32 k) const { return { x * k, y * k }; }
    constexpr uint2 operator/(const u32 k) const { return { x / k, y / k }; }
    constexpr uint2& operator+=(const uint2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr uint2& operator-=(const uint2 other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    constexpr uint2& operator*=(const uint2 other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }
    constexpr uint2& operator/=(const uint2 other) {
        x /= other.x;
        y /= other.y;
        return *this;
    }
    constexpr b8 operator==(const uint2 other) const { return x == other.x && y == other.y; }
    constexpr b8 operator!=(const uint2 other) const { return !(*this == other); }
};
struct uint4 {
    u32 x;
    u32 y;
    u32 z;
    u32 w;
    constexpr uint4 operator+(const uint4 other) const { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
    constexpr uint4 operator-(const uint4 other) const { return { x - other.x, y - other.y, z - other.z, w - other.w }; }
    constexpr uint4 operator*(const uint4 other) const { return { x * other.x, y * other.y, z * other.z, w * other.w }; }
    constexpr uint4 operator/(const uint4 other) const { return { x / other.x, y / other.y, z / other.z, w / other.w }; }
    constexpr uint4 operator*(const u32 k) const { return { x * k, y * k, z * k, w * k }; }
    constexpr uint4 operator/(const u32 k) const { return { x / k, y / k, z / k, w / k }; }
    constexpr uint4& operator+=(const uint4 other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }
    constexpr uint4& operator-=(const uint4 other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }
    constexpr uint4& operator*=(const uint4 other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }
    constexpr uint4& operator/=(const uint4 other) {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }

    constexpr b8 operator==(const uint4 other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
    constexpr b8 operator!=(const uint4 other) const { return !(*this == other); }
    // constexpr u32& operator[](u32 pos) { return *std::array { &x, &y, &z, &w }[pos]; }
};
struct float2 {
    f32 x;
    f32 y;
    constexpr float2 operator+(const float2 other) const { return float2 { x + other.x, y + other.y }; }
    constexpr float2 operator-(const float2 other) const { return float2 { x - other.x, y - other.y }; }
    constexpr float2 operator*(const float2 other) const { return float2 { x * other.x, y * other.y }; }
    constexpr float2 operator/(const float2 other) const { return float2 { x / other.x, y / other.y }; }
    constexpr float2 operator*(const f32 k) const { return float2 { x * k, y * k }; }
    constexpr float2 operator/(const f32 k) const { return float2 { x / k, y / k }; }
    constexpr float2& operator+=(const float2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr float2& operator-=(const float2 other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    constexpr float2& operator*=(const float2 other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }
    constexpr float2& operator/=(const float2 other) {
        x /= other.x;
        y /= other.y;
        return *this;
    }
    constexpr b8 operator==(const float2 other) const { return x == other.x && y == other.y; }
    constexpr b8 operator!=(const float2 other) const { return !(*this == other); }
};
constexpr u8 U8_MAX = UINT8_MAX;
constexpr u32 U32_MAX = UINT32_MAX;

template <class T, class U> concept Derived = std::is_base_of_v<U, T>;

struct Entity {
    explicit constexpr Entity(const u32 index) : index(index) { }
    constexpr Entity() : index(U32_MAX) { }
    static const Entity NONE;

    operator u32() const { return index; } // NOLINT(*-explicit-constructor)

    b8 operator!=(const Entity other) const { return index != other.index; }
    b8 operator==(const Entity other) const { return index == other.index; }
    Entity& operator++() {
        index++;
        return *this;
    }
    Entity operator*() const { return *this; }

protected:
    u32 index;
};

template <class> struct Handle {
    u32 id;
    explicit constexpr Handle(const u32 id) : id(id) { }
    b8 operator==(const Handle&) const = default;
};

template <class T> struct OptionalHandle {
    constexpr OptionalHandle() noexcept : id(U32_MAX) { }
    constexpr OptionalHandle(std::nullopt_t) noexcept : id(U32_MAX) { }
    constexpr explicit OptionalHandle(const u32 value) noexcept : id(value) { }
    constexpr OptionalHandle(const Handle<T> handle) noexcept : id(handle.id) { }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return id != U32_MAX; }
    [[nodiscard]] constexpr Handle<T> GetHandle() const noexcept { return Handle<T> { id }; }
    constexpr void Reset() noexcept { id = U32_MAX; }
    u32 id;
};

inline const Entity Entity::NONE = Entity { };

template <Derived<Entity> T = Entity> struct OptionalEntity {
    constexpr operator uint2() const { return entity; }
    constexpr OptionalEntity() = default;
    constexpr explicit OptionalEntity(T entity) : entity(entity) { }
    [[nodiscard]] constexpr b8 IsNone() const { return entity == Entity::NONE; }
    [[nodiscard]] constexpr b8 IsSome() const { return entity != Entity::NONE; }
    [[nodiscard]] constexpr T Entity() const { return entity; }

private:
    T entity { Entity::NONE };
};

struct Archetype {
    u32 Count { 0U };
    virtual Entity Add() { return Entity { Count++ }; }
    virtual b8 Remove(Entity entity) {
        if (Count == 0U) { return false; }
        Count--;
        return true;
    }

    [[nodiscard]] static constexpr Entity begin() noexcept { return Entity { 0U }; }
    [[nodiscard]] constexpr Entity end() const noexcept { return Entity { Count }; }
};
