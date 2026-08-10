export module pce.util;

import std;
import pce.std;
import pce.collections;

export namespace hex {
const char* NumberToOrdinal(u32 number) {
    switch (number % 100) {
        case 11:
        case 12:
        case 13: return "th";
        default:
            switch (number % 10) {
                case 1: return "st";
                case 2: return "nd";
                case 3: return "rd";
                default: return "th";
            }
    }
}

String NumberToRomanNumerals(u32 num) {
    constexpr std::array<std::pair<u32, const char*>, 13> ROMAN_NUMERALS { { { 1000, "M" }, { 900, "CM" }, { 500, "D" }, { 400, "CD" }, { 100, "C" }, { 90, "XC" }, { 50, "L" }, { 40, "XL" }, { 10, "X" }, { 9, "IX" }, { 5, "V" }, { 4, "IV" }, { 1, "I" } } };

    if (num < 1 || num > 3999) { return std::to_string(num); }
    String romans;
    for (const auto& [value, symbol] : ROMAN_NUMERALS) {
        while (num >= value) {
            romans += symbol;
            num -= value;
        }
    }
    return romans;
}
} // namespace hex
