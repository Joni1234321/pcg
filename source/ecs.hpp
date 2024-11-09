// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppNonExplicitConvertingConstructor
#pragma once

#include "collections.hpp"
#include "types.hpp"

namespace pcg {
template <typename T, typename Parameter, template<typename> class... Skills> class NamedType : public Skills<NamedType<T, Parameter, Skills...>>... {
public:
    constexpr explicit NamedType(const T& value) : value(value) { }
    T& Value() { return value; }
    const T& Value() const { return value; }

private:
    T value;
};
template <typename T, template<typename> class crtpType> struct crtp {
    T& Underlying() { return static_cast<T&>(*this); }
    const T& Underlying() const { return static_cast<const T&>(*this); }
};
template <typename T> struct Arithmetic : crtp<T, Arithmetic> {
    T operator+(const T& other) const { return T(this->Underlying().Value() + other.Value()); }
    T operator-(const T& other) const { return T(this->Underlying().Value() - other.Value()); }
    T operator*(const T& other) const { return T(this->Underlying().Value() * other.Value()); }
    T operator/(const T& other) const { return T(this->Underlying().Value() / other.Value()); }
    T& operator+=(const T& other) {
        this->Underlying().Value() += other.Value();
        return this->Underlying();
    }
    T& operator-=(const T& other) {
        this->Underlying().Value() -= other.Value();
        return this->Underlying();
    }
    T& operator*=(const T& other) {
        this->Underlying().Value() *= other.Value();
        return this->Underlying();
    }
    T& operator/=(const T& other) {
        this->Underlying().Value() /= other.Value();
        return this->Underlying();
    }
    T operator++(int) {
        T old = *this->Underlying();
        this->Underlying().Value()++;
        return old;
    }
    T& operator++() {
        this->Underlying().Value()++;
        return this->Underlying();
    }
    // Comparison operators
    b8 operator==(const T& other) const { return this->Underlying().Value() == other.Value(); }
    b8 operator!=(const T& other) const { return this->Underlying().Value() != other.Value(); }
    b8 operator<(const T& other) const { return this->Underlying().Value() < other.Value(); }
    b8 operator<=(const T& other) const { return this->Underlying().Value() <= other.Value(); }
    b8 operator>(const T& other) const { return this->Underlying().Value() > other.Value(); }
    b8 operator>=(const T& other) const { return this->Underlying().Value() >= other.Value(); }
};

template <typename T> struct Incrementable : crtp<T, Incrementable> {
    T& operator+=(const T& other) {
        this->Underlying().Value() += other.Value();
        return this->Underlying();
    }
};
template <typename T> struct Printable : crtp<T, Printable> {
    void print(std::ostream& os) const { os << this->Underlying().Value(); }
};

template <typename T, typename Parameter> std::ostream& operator<<(std::ostream& os, const NamedType<T, Parameter>& object) {
    object.print(os);
    return os;
}
struct Archetype {
    u32 Count { 0U };
    virtual b8 Add() {
        Count++;
        return true;
    }
    virtual b8 Remove(Entity entity) {
        if (Count == 0U) { return false; }
        Count--;
        return true;
    };

    [[nodiscard]] static constexpr Entity begin() noexcept { return Entity { 0U }; }
    [[nodiscard]] constexpr Entity end() const noexcept { return Entity { Count }; }
};
} // namespace pcg

#include "format"
template <typename T, typename Parameter, template <typename> class... Skills> struct std::formatter<pcg::NamedType<T, Parameter, Skills...>> : std::formatter<T> {
    auto format(const pcg::NamedType<T, Parameter, Skills...>& data, std::format_context& ctx) const { return std::formatter<T>::format(data.Value(), ctx); }
};
template <typename EnumType> requires std::is_enum_v<EnumType>
struct std::formatter<EnumType> : std::formatter<std::underlying_type_t<EnumType>> {
    auto format(const EnumType& enum_value, format_context& ctx) const { return std::formatter<std::underlying_type_t<EnumType>>::format(static_cast<std::underlying_type_t<EnumType>>(enum_value), ctx); }
};
template < > struct std::formatter<pce::String> : std::formatter<const char*> {
    auto format(const pce::String& data, std::format_context& ctx) const { return formatter<const char*>::format(data.CString(), ctx); }
};
template < > struct std::formatter<Entity> : std::formatter<u32> {
    auto format(const Entity& data, std::format_context& ctx) const { return formatter<u32>::format(data, ctx); }
};
