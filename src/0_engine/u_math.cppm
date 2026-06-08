module;

export module pce.math;

import std;
import pce.std;

export namespace pce {
template <std::integral T> constexpr T saturating_sub(T a, T b) noexcept {
    T result;
    if (!__builtin_sub_overflow(a, b, &result)) { return result; }
    if constexpr (std::is_unsigned_v<T>) { return std::numeric_limits<T>::min(); }
    return a < 0 ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
}
}
export namespace pce::math {
 constexpr f32 PI = std::numbers::pi_v<f32>;
 constexpr f32 SQRT_3 = std::numbers::sqrt3_v<float>;
 constexpr f32 DEG_2_RAD = PI / 180.0F;
[[nodiscard]] constexpr f32 Sqrt(const f32 val) { return std::sqrtf(val); }
[[nodiscard]] constexpr f32 Square(const f32 val) { return val * val; }
[[nodiscard]] inline f32 Sin(const f32 t) { return std::sinf(t); }
[[nodiscard]] inline f32 Cos(const f32 t) { return std::cosf(t); }
[[nodiscard]] inline f32 Atan2(const f32 x, const f32 y) { return std::atan2f(x, y); }
[[nodiscard]] constexpr f32 Pow(const f32 x, const f32 y) { return std::powf(x, y); }
[[nodiscard]] constexpr u32 Pow(const u32 x, const u32 y) { return static_cast<u32>(std::powf(static_cast<f32>(x), static_cast<f32>(y))); }
constexpr std::pair<u32, u32> Div(const u32 value, const u32 divisor) { return { value / divisor, value % divisor }; }
template <typename T = void> T Sub(const T& left, const T& right) { return left - right; }
template <typename T = void> struct Minus {
    T operator()(const T& left, const T& right) const { return left - right; } // NOLINT(*-overloaded-operator)
};
template <typename T = void> struct Plus {
    T operator()(const T& left, const T& right) const { return left + right; } // NOLINT(*-overloaded-operator)
};
template <typename T = void> struct Size {
    u32 operator()(const T& container) const { return container.size(); } // NOLINT(*-overloaded-operator)
};
template <typename T> [[nodiscard]] constexpr T Max(T left, T right) { return left > right ? left : right; }
template <typename T> [[nodiscard]] constexpr T Min(T left, T right) { return left < right ? left : right; }
template <typename T> [[nodiscard]] constexpr u32 FloorToU32(const T value) { return static_cast<u32>(value); }
[[nodiscard]] inline f32 Ceil(const f32 value) { return std::ceilf(value); }
[[nodiscard]] inline f32 Floor(const f32 value) { return std::floorf(value); }
[[nodiscard]] constexpr f32 Clamp(const f32 value, const f32 min, const f32 max) { return std::clamp(value, min, max); }
[[nodiscard]] constexpr i32 Round(f32 a) { return static_cast<i32>(std::roundf(a)); }
template <typename T> [[nodiscard]] constexpr T Abs(const T value) { return value < 0 ? -value : value; }
template <typename T> [[nodiscard]] constexpr T Lerp(const T a, const T b, const f32 t) { return t * (b - a) + a; }
[[nodiscard]] constexpr f32 Hypot(const f32 x, const f32 y) { return std::hypot(x, y); }
[[nodiscard]] constexpr f32 Hypot(const f32 x, const f32 y, const f32 z) { return std::hypot(x, y, z); }
[[nodiscard]] constexpr f32 Hypot(const f32 x, const f32 y, const f32 z, const f32 w) { return Sqrt(x * x + y * y + z * z + w * w); }

template <class _Ty> [[nodiscard]] constexpr const _Ty& max2(const _Ty& _Left, const _Ty& _Right) noexcept(noexcept(_Left < _Right)) { return _Left < _Right ? _Right : _Left; }
template <class T> struct max {
    [[nodiscard]] constexpr const T& operator()(const T& left, const T& right) const noexcept(noexcept(left < right)) { return left < right ? right : left; }
};
template <typename T> [[nodiscard]] constexpr Vec2<T> Abs(Vec2<T> v) { return { Abs(v.x), Abs(v.y) }; }
template <typename T> [[nodiscard]] constexpr Vec3<T> Abs(Vec3<T> v) { return { Abs(v.x), Abs(v.y), Abs(v.z) }; }
template <typename T> [[nodiscard]] constexpr Vec4<T> Abs(Vec4<T> v) { return { Abs(v.x), Abs(v.y), Abs(v.z), Abs(v.w) }; }
template <typename T> [[nodiscard]] constexpr Vec2<T> Lerp(Vec2<T> a, Vec2<T> b, f32 t) { return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) }; }
template <typename T> [[nodiscard]] constexpr Vec3<T> Lerp(Vec3<T> a, Vec3<T> b, f32 t) { return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t) }; }
template <typename T> [[nodiscard]] constexpr Vec4<T> Lerp(Vec4<T> a, Vec4<T> b, f32 t) { return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t), Lerp(a.w, b.w, t) }; }
template <typename T> [[nodiscard]] constexpr f32 Hypot(Vec2<T> v) { return Hypot(v.x, v.y); }
template <typename T> [[nodiscard]] constexpr f32 Hypot(Vec3<T> v) { return Hypot(v.x, v.y, v.z); }
template <typename T> [[nodiscard]] constexpr f32 Hypot(Vec4<T> v) { return Hypot(v.x, v.y, v.z, v.w); }
template <typename T> [[nodiscard]] constexpr f32 LengthSq(Vec2<T> v) { return v.x * v.x + v.y * v.y; }
template <typename T> [[nodiscard]] constexpr f32 LengthSq(Vec3<T> v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
template <typename T> [[nodiscard]] constexpr f32 LengthSq(Vec4<T> v) { return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w; }
template <typename T> [[nodiscard]] constexpr f32 CMin(Vec2<T> v) { return Min(v.x, v.y); }
template <typename T> [[nodiscard]] constexpr f32 CMin(Vec3<T> v) { return Min(Min(v.x, v.y), v.z); }
template <typename T> [[nodiscard]] constexpr f32 CMin(Vec4<T> v) { return Min(Min(v.x, v.y), Min(v.z, v.w)); }
template <typename T> [[nodiscard]] constexpr f32 Min(Vec2<T> a, Vec2<T> b) { return { Min(a.x, b.x), Min(a.y, b.y) }; }
template <typename T> [[nodiscard]] constexpr f32 Min(Vec3<T> a, Vec3<T> b) { return { Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z) }; }
template <typename T> [[nodiscard]] constexpr f32 Min(Vec4<T> a, Vec4<T> b) { return { Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z), Min(a.w, b.w) }; }

template <typename T> [[nodiscard]] constexpr T SaturatingSub(const T val, const T sub) { return val > sub ? val - sub : T { 0 }; }

inline float2 Rotate(float2 point, f32 theta) {
    const f32 cos = Cos(theta);
    const f32 sin = Sin(theta);
    const f32 x = point.x * cos - point.y * sin;
    const f32 y = point.x * sin + point.y * cos;
    return float2 { x, y };
}
} // namespace pce::math
