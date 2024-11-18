// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppNonExplicitConvertingConstructor
#pragma once

#include "collections.hpp"
#include "types.hpp"

namespace pcg {
template <typename T, typename TagType, template<typename> class... InheritList> class NamedType : public InheritList<NamedType<T, TagType, InheritList...>>... {
public:
    using ValueType = T;
    using Tag = TagType;
    constexpr explicit NamedType(const T& value) : value(value) { }
    T& Value() { return value; }
    const T& Value() const { return value; }
    explicit operator T() const { return Value(); };

private:
    T value;
};
template <typename DerivedType, template<typename> class Recurring> struct RecurringDerived {
    DerivedType& Derived() { return static_cast<DerivedType&>(*this); }
    const DerivedType& Derived() const { return static_cast<const DerivedType&>(*this); }
};

template <typename T, template<typename> class Skill>concept HasASkill = std::derived_from<T, Skill<T>>;

template <typename T> struct FormatLongNumber : RecurringDerived<T, FormatLongNumber> { };
template <typename T> struct Arithmetic : RecurringDerived<T, Arithmetic> {
    T operator+(const T& other) const { return T(this->Derived().Value() + other.Value()); }
    T operator-(const T& other) const { return T(this->Derived().Value() - other.Value()); }
    T operator*(const T& other) const { return T(this->Derived().Value() * other.Value()); }
    T operator/(const T& other) const { return T(this->Derived().Value() / other.Value()); }
    // Equals operators
    T& operator+=(const T& other) {
        this->Derived().Value() += other.Value();
        return this->Derived();
    }
    T& operator-=(const T& other) {
        this->Derived().Value() -= other.Value();
        return this->Derived();
    }
    T& operator*=(const T& other) {
        this->Derived().Value() *= other.Value();
        return this->Derived();
    }
    T& operator/=(const T& other) {
        this->Derived().Value() /= other.Value();
        return this->Derived();
    }
    T operator++(int) {
        T old = *this->Derived();
        (void)this->Derived().Value()++;
        return old;
    }
    T& operator++() {
        (void)this->Derived().Value()++;
        return this->Derived();
    }
    // Comparison operators
    b8 operator==(const T& other) const { return this->Derived().Value() == other.Value(); }
    b8 operator!=(const T& other) const { return this->Derived().Value() != other.Value(); }
    b8 operator<(const T& other) const { return this->Derived().Value() < other.Value(); }
    b8 operator<=(const T& other) const { return this->Derived().Value() <= other.Value(); }
    b8 operator>(const T& other) const { return this->Derived().Value() > other.Value(); }
    b8 operator>=(const T& other) const { return this->Derived().Value() >= other.Value(); }
};
template <typename T> struct Printable : RecurringDerived<T, Printable> {
    void print(std::ostream& os) const { os << this->Derived().Value(); }
};
template <typename T, typename Parameter> std::ostream& operator<<(std::ostream& os, const NamedType<T, Parameter>& object) {
    object.print(os);
    return os;
}
} // namespace pcg

#include "format"
template <typename T, typename Parameter, template <typename> class... Skills> struct std::formatter<pcg::NamedType<T, Parameter, Skills...>> : std::formatter<T> {
    auto format(const pcg::NamedType<T, Parameter, Skills...>& data, std::format_context& ctx) const { return std::formatter<T>::format(data.Value(), ctx); }
};
template <typename EnumType> requires std::is_enum_v<EnumType>
struct std::formatter<EnumType> : std::formatter<std::underlying_type_t<EnumType>> {
    auto format(const EnumType& enum_value, format_context& ctx) const { return std::formatter<std::underlying_type_t<EnumType>>::format(static_cast<const std::underlying_type_t<EnumType>>(enum_value), ctx); }
};
template < > struct std::formatter<pce::String> : std::formatter<const char*> {
    auto format(const pce::String& data, std::format_context& ctx) const { return formatter<const char*>::format(data.CString(), ctx); }
};
template < > struct std::formatter<Entity> : std::formatter<u32> {
    auto format(const Entity& data, std::format_context& ctx) const { return formatter<u32>::format(static_cast<const u32>(data), ctx); }
};
