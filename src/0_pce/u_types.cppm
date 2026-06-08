module;
// NOLINTBEGIN(*-include-cleaner)
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <new>
#include <utility>
#include <vector>
#include <limits>
#include <numbers>
// NOLINTEND(*-include-cleaner)

#if defined(_DEBUG) || defined(NDEBUG)
    #define DEBUG
#endif // DEBUG
export module pce.std;

export using i8 = int8_t;
export using u8 = uint8_t;
export using i16 = int16_t;
export using u16 = uint16_t;
export using i32 = int32_t;
export using u32 = uint32_t;
export using i64 = int64_t;
export using u64 = uint64_t;
export using usize = size_t;

export using f32 = float;
export using f64 = double;

export using c8 = char8_t;
export using c16 = char16_t;
export using c32 = char32_t;
export using b8 = bool;

export template <class T> using Optional = std::optional<T>;

export template <typename T> struct Vec2 {
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
    constexpr Vec2& operator+=(const Vec2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr Vec2& operator-=(const Vec2 other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    constexpr Vec2& operator*=(const Vec2 other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }
    constexpr Vec2& operator/=(const Vec2 other) {
        x /= other.x;
        y /= other.y;
        return *this;
    }
    constexpr b8 operator==(const Vec2 other) const { return x == other.x && y == other.y; }
    constexpr b8 operator!=(const Vec2 other) const { return !(*this == other); }
};

export template <typename T> struct Vec3 {
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
    constexpr Vec3& operator+=(const Vec3 other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    constexpr Vec3& operator-=(const Vec3 other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    constexpr Vec3& operator*=(const Vec3 other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }
    constexpr Vec3& operator/=(const Vec3 other) {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        return *this;
    }
    constexpr b8 operator==(const Vec3 other) const { return x == other.x && y == other.y && z == other.z; }
    constexpr b8 operator!=(const Vec3 other) const { return !(*this == other); }
};

export template <typename T> struct Vec4 {
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
    constexpr Vec4& operator+=(const Vec4 other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }
    constexpr Vec4& operator-=(const Vec4 other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }
    constexpr Vec4& operator*=(const Vec4 other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }
    constexpr Vec4& operator/=(const Vec4 other) {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }
    constexpr b8 operator==(const Vec4 other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
    constexpr b8 operator!=(const Vec4 other) const { return !(*this == other); }
};

export using uint2 = Vec2<u32>;
export using uint3 = Vec3<u32>;
export using uint4 = Vec4<u32>;
export using int2 = Vec2<i32>;
export using int3 = Vec3<i32>;
export using float2 = Vec2<f32>;
export using float3 = Vec3<f32>;

export constexpr u8 U8_MAX = UINT8_MAX;
export constexpr u32 U32_MAX = UINT32_MAX;

export template <class T, class U> concept Derived = std::is_base_of_v<U, T>;

export struct Entity {
    explicit constexpr Entity(const u32 index) : index(index) { }
    constexpr Entity() : index(U32_MAX) { }
    static const Entity NONE;

    operator u32() const { return index; }

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

export template <class> struct Handle {
    u32 id;
    explicit constexpr Handle(const u32 id) : id(id) { }
    b8 operator==(const Handle&) const = default;
};
export template <class T> struct HandleOptional {
    constexpr HandleOptional() noexcept : id(U32_MAX) { }
    constexpr HandleOptional(std::nullopt_t) noexcept : id(U32_MAX) { }
    constexpr explicit HandleOptional(const u32 value) noexcept : id(value) { }
    constexpr HandleOptional(const Handle<T> handle) noexcept : id(handle.id) { }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return id != U32_MAX; }
    [[nodiscard]] constexpr Handle<T> GetHandle() const noexcept { return Handle<T> { id }; }
    constexpr void Reset() noexcept { id = U32_MAX; }
    b8 operator==(const HandleOptional&) const = default;
    u32 id;
};
export template <class T> struct std::hash<Handle<T>> {
    usize operator()(const Handle<T>& handle) const noexcept { return std::hash<u32> { }(handle.id); }
};
export template <class T> struct std::hash<HandleOptional<T>> {
    usize operator()(const HandleOptional<T>& handle_optional) const noexcept { return std::hash<u32> { }(handle_optional.id); }
};

inline const Entity Entity::NONE = Entity { };

export template <Derived<Entity> T = Entity> struct OptionalEntity {
    constexpr operator uint2() const { return entity; }
    constexpr OptionalEntity() = default;
    constexpr explicit OptionalEntity(T entity) : entity(entity) { }
    [[nodiscard]] constexpr b8 IsNone() const { return entity == Entity::NONE; }
    [[nodiscard]] constexpr b8 IsSome() const { return entity != Entity::NONE; }
    [[nodiscard]] constexpr T Entity() const { return entity; }

private:
    T entity { Entity::NONE };
};

export struct Archetype {
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

export template <class T> constexpr void HashCombine(usize& seed, const T& v) noexcept { seed ^= std::hash<T> { }(v) + 0x9e3779b9u + (seed << 6) + (seed >> 2); }

export template <class T> struct std::hash<Vec2<T>> {
    usize operator()(const Vec2<T> vec) const noexcept {
        size_t h = std::hash<T> { }(vec.x);
        HashCombine(h, vec.y);
        return h;
    }
};
export template <class T> struct std::hash<Vec3<T>> {
    usize operator()(const Vec3<T> vec) const noexcept {
        size_t h = std::hash<T> { }(vec.x);
        HashCombine(h, vec.y);
        HashCombine(h, vec.z);
        return h;
    }
};
export template <class T> struct std::hash<Vec4<T>> {
    usize operator()(const Vec4<T> vec) const noexcept {
        size_t h = std::hash<T> { }(vec.x);
        HashCombine(h, vec.y);
        HashCombine(h, vec.z);
        HashCombine(h, vec.w);
        return h;
    }
};
