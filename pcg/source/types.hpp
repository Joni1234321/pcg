#pragma once

using i8 = char;
using u8 = unsigned char;
using i16 = short;
using u16 = unsigned short;
using i32 = int;
using u32 = unsigned int;
using i64 = long long;
using u64 = unsigned long long;
using usize = size_t;

using f32 = float;
using f64 = double;

using c8 = char;
using c16 = char16_t;
using c32 = char32_t;
using b8 = bool;

struct Entity {
  u32 index;
  constexpr Entity() : index(static_cast<u32>(-1)) {}
  constexpr Entity(u32 v) : index(v) {}

  bool operator!=(const Entity other) { return index != other.index; }
  Entity &operator++() { index++; return *this; }
  Entity operator*() { return *this; }
};
