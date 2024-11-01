#pragma once 
#include <format>
#include "components.hpp"

template <>
struct std::formatter<pcg::Money> : std::formatter<f32> {
	auto format(const pcg::Money& data, std::format_context& ctx) const {
		return formatter<f32>::format(data.Value, ctx);
	}
};
template <typename EnumType> requires std::is_enum_v<EnumType>
struct std::formatter<EnumType> : std::formatter<std::underlying_type_t<EnumType>>
{
	auto format(const EnumType& enumValue, format_context& ctx) const
	{
		return std::formatter<std::underlying_type_t<EnumType>>::format(
			static_cast<std::underlying_type_t<EnumType>>(enumValue), ctx);
	}
};