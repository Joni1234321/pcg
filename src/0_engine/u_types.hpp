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

using i8 = int8_t;    // NOLINT(*-identifier-naming)
using u8 = uint8_t;   // NOLINT(*-identifier-naming)
using i16 = int16_t;  // NOLINT(*-identifier-naming)
using u16 = uint16_t; // NOLINT(*-identifier-naming)
using i32 = int32_t;  // NOLINT(*-identifier-naming)
using u32 = uint32_t; // NOLINT(*-identifier-naming)
using i64 = int64_t;  // NOLINT(*-identifier-naming)
using u64 = uint64_t; // NOLINT(*-identifier-naming)
using usize = size_t; // NOLINT(*-identifier-naming)

using f32 = float;  // NOLINT(*-identifier-naming)
using f64 = double; // NOLINT(*-identifier-naming)

using c8 = char8_t;   // NOLINT(*-identifier-naming)
using c16 = char16_t; // NOLINT(*-identifier-naming)
using c32 = char32_t; // NOLINT(*-identifier-naming)
using b8 = bool;      // NOLINT(*-identifier-naming)

template <typename T> struct Vec2 {
    T x;
    T y;
    constexpr Vec2() = default;
    constexpr explicit Vec2(T k) : x(k), y(k) { }
    constexpr Vec2(T x, T y) : x(x), y(y) { }
    template <typename U> explicit constexpr Vec2(const Vec2<U> v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)) { }
    constexpr Vec2 operator+(const Vec2 other) const { return { x + other.x, y + other.y }; }
    constexpr Vec2 operator-(const Vec2 other) const { return { x - other.x, y - other.y }; }
    constexpr Vec2 operator*(const Vec2 other) const { return { x * other.x, y * other.y }; }
    constexpr Vec2 operator/(const Vec2 other) const { return { x / other.x, y / other.y }; }
    // clang-format off
    constexpr Vec2& operator+=(const Vec2 other) { x += other.x; y += other.y; return *this; }
    constexpr Vec2& operator-=(const Vec2 other) { x -= other.x; y -= other.y; return *this; }
    constexpr Vec2& operator*=(const Vec2 other) { x *= other.x; y *= other.y; return *this; }
    constexpr Vec2& operator/=(const Vec2 other) { x /= other.x; y /= other.y; return *this; }
    // clang-format on
    constexpr b8 operator==(const Vec2 other) const { return x == other.x && y == other.y; }
    constexpr b8 operator!=(const Vec2 other) const { return !(*this == other); }
};

template <typename T> struct Vec3 {
    T x;
    T y;
    T z;
    constexpr Vec3() = default;
    constexpr explicit Vec3(T k) : x(k), y(k), z(k) { }
    constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) { }
    template <typename U> explicit constexpr Vec3(const Vec3<U> v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)) { }
    constexpr Vec3 operator+(const Vec3 other) const { return { x + other.x, y + other.y, z + other.z }; }
    constexpr Vec3 operator-(const Vec3 other) const { return { x - other.x, y - other.y, z - other.z }; }
    constexpr Vec3 operator*(const Vec3 other) const { return { x * other.x, y * other.y, z * other.z }; }
    constexpr Vec3 operator/(const Vec3 other) const { return { x / other.x, y / other.y, z / other.z }; }
    // clang-format off
    constexpr Vec3& operator+=(const Vec3 other) { x += other.x; y += other.y; z += other.z; return *this; }
    constexpr Vec3& operator-=(const Vec3 other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    constexpr Vec3& operator*=(const Vec3 other) { x *= other.x; y *= other.y; z *= other.z; return *this; }
    constexpr Vec3& operator/=(const Vec3 other) { x /= other.x; y /= other.y; z /= other.z; return *this; }
    // clang-format on
    constexpr b8 operator==(const Vec3 other) const { return x == other.x && y == other.y && z == other.z; }
    constexpr b8 operator!=(const Vec3 other) const { return !(*this == other); }
};

template <typename T> struct Vec4 {
    T x;
    T y;
    T z;
    T w;
    constexpr Vec4() = default;
    constexpr explicit Vec4(T k) : x(k), y(k), z(k), w(k) { }
    constexpr Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
    template <typename U> explicit constexpr Vec4(const Vec4<U> v) : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)), w(static_cast<T>(v.w)) { }
    constexpr Vec4 operator+(const Vec4 other) const { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
    constexpr Vec4 operator-(const Vec4 other) const { return { x - other.x, y - other.y, z - other.z, w - other.w }; }
    constexpr Vec4 operator*(const Vec4 other) const { return { x * other.x, y * other.y, z * other.z, w * other.w }; }
    constexpr Vec4 operator/(const Vec4 other) const { return { x / other.x, y / other.y, z / other.z, w / other.w }; }
    // clang-format off
    constexpr Vec4& operator+=(const Vec4 other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
    constexpr Vec4& operator-=(const Vec4 other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
    constexpr Vec4& operator*=(const Vec4 other) { x *= other.x; y *= other.y; z *= other.z; w *= other.w; return *this; }
    constexpr Vec4& operator/=(const Vec4 other) { x /= other.x; y /= other.y; z /= other.z; w /= other.w; return *this; }
    // clang-format on
    constexpr b8 operator==(const Vec4 other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
    constexpr b8 operator!=(const Vec4 other) const { return !(*this == other); }
};

using uint2 = Vec2<u32>;  // NOLINT(*-identifier-naming)
using uint3 = Vec3<u32>;  // NOLINT(*-identifier-naming)
using uint4 = Vec4<u32>;  // NOLINT(*-identifier-naming)
using int2 = Vec2<i32>;   // NOLINT(*-identifier-naming)
using int3 = Vec3<i32>;   // NOLINT(*-identifier-naming)
using float2 = Vec2<f32>; // NOLINT(*-identifier-naming)
using float3 = Vec3<f32>; // NOLINT(*-identifier-naming)

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
        (void)entity;
        if (Count == 0U) { return false; }
        Count--;
        return true;
    }

    [[nodiscard]] static constexpr Entity begin() noexcept { return Entity { 0U }; }
    [[nodiscard]] constexpr Entity end() const noexcept { return Entity { Count }; }
};
