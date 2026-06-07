module;

#include <SDL3/SDL_timer.h>
#include <format>

export module pce.u_ecs;

// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppNonExplicitConvertingConstructor

import pce.std;
import pce.collections;

export namespace pce {
template <typename T, typename TagType, template <typename> class... InheritList> struct StrongType : InheritList<StrongType<T, TagType, InheritList...>>... {
    T value;
    constexpr explicit StrongType(const T& value) : value(value) { }
};
template <typename DerivedType, template <typename> class> struct RecurringDerived {
    DerivedType& Derived() { return static_cast<DerivedType&>(*this); }
    const DerivedType& Derived() const { return static_cast<const DerivedType&>(*this); }
};

template <typename T> struct FormatLongNumber : RecurringDerived<T, FormatLongNumber> { };
template <typename T> struct Arithmetic : RecurringDerived<T, Arithmetic> {
    constexpr T operator+(const T& other) const { return T(this->Derived().value + other.value); }
    constexpr T operator-(const T& other) const { return T(this->Derived().value - other.value); }
    constexpr T operator*(const T& other) const { return T(this->Derived().value * other.value); }
    constexpr T operator/(const T& other) const { return T(this->Derived().value / other.value); }
    // Equals operators
    constexpr T& operator+=(const T& other) {
        this->Derived().value += other.value;
        return this->Derived();
    }
    constexpr T& operator-=(const T& other) {
        this->Derived().value -= other.value;
        return this->Derived();
    }
    constexpr T& operator*=(const T& other) {
        this->Derived().value *= other.value;
        return this->Derived();
    }
    constexpr T& operator/=(const T& other) {
        this->Derived().value /= other.value;
        return this->Derived();
    }
    constexpr T operator++(int) {
        T old = *this->Derived();
        (void)this->Derived().value++;
        return old;
    }
    constexpr T& operator++() {
        (void)this->Derived().value++;
        return this->Derived();
    }
    // Comparison operators
    constexpr b8 operator==(const T& other) const { return this->Derived().value == other.value; }
    constexpr b8 operator!=(const T& other) const { return this->Derived().value != other.value; }
    constexpr b8 operator<(const T& other) const { return this->Derived().value < other.value; }
    constexpr b8 operator<=(const T& other) const { return this->Derived().value <= other.value; }
    constexpr b8 operator>(const T& other) const { return this->Derived().value > other.value; }
    constexpr b8 operator>=(const T& other) const { return this->Derived().value >= other.value; }
};
template <typename T> struct Printable : RecurringDerived<T, Printable> {
    void print(std::ostream& os) const { os << this->Derived().value; }
};
template <typename T, typename Parameter> std::ostream& operator<<(std::ostream& os, const StrongType<T, Parameter>& object) {
    object.print(os);
    return os;
}
using seconds32 = StrongType<u32, struct SecondsTag, Arithmetic>;
using miliseconds32 = StrongType<u32, struct MilisecondsTag, Arithmetic>;
using nanoseconds64 = StrongType<u64, struct Nanoseconds64Tag, Arithmetic>;
inline miliseconds32 TimeNowMS() noexcept { return miliseconds32 { static_cast<u32>(SDL_GetTicks()) }; }
inline nanoseconds64 TimeNowNS() noexcept { return nanoseconds64 { SDL_GetTicksNS() }; }

 constexpr f32 SECONDS_TO_MS = 1'000.0F;
 constexpr f32 MS_TO_SECONDS = 1.0F / 1'000.0F;
 constexpr f32 SECONDS_TO_NS = 1'000'000'000.0F;
 constexpr f32 NS_TO_SECONDS = 1.0F / 1'000'000'000.0F;

} // namespace pce

template <typename T, typename Parameter, template <typename> class... Skills> struct std::formatter<pce::StrongType<T, Parameter, Skills...>> : std::formatter<T> {
    auto format(const pce::StrongType<T, Parameter, Skills...>& data, std::format_context& ctx) const { return std::formatter<T>::format(data.value, ctx); }
};
template <typename EnumType> requires std::is_enum_v<EnumType> struct std::formatter<EnumType> : std::formatter<std::underlying_type_t<EnumType>> {
    auto format(const EnumType& enum_value, format_context& ctx) const { return std::formatter<std::underlying_type_t<EnumType>>::format(static_cast<const std::underlying_type_t<EnumType>>(enum_value), ctx); }
};
template <> struct std::formatter<pce::String> : std::formatter<const char*> {
    auto format(const pce::String& data, std::format_context& ctx) const { return formatter<const char*>::format(data.c_str(), ctx); }
};
template <> struct std::formatter<Entity> : std::formatter<u32> {
    auto format(const Entity& data, std::format_context& ctx) const { return formatter<u32>::format(static_cast<u32>(data), ctx); }
};

