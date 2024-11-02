#pragma once

#include <vector>
#include "types.hpp"
#include <memory>
#include <stdexcept>

namespace pce {
	namespace util {
		template <typename T>
		constexpr void SwapPop(std::vector<T>& v, const std::vector<Entity>& entities) {
			const size_t n = entities.size();
			for (u32 i = 0; i < n; i++) {
				const u32 idx = entities[n - i - 1].index;
				std::swap(v[idx], v.back());
				v.pop_back();
			}
		}

		template <typename T>
		constexpr void SwapPop(std::vector<T>& v, u32 i) {
			std::swap(v[i], v.back());
			v.pop_back();
		}

		template <typename Collection>
		const typename Collection::key_type RandomKey(const Collection& collection) {
			if (std::empty(collection)) throw std::runtime_error("Collection is empty!");

			auto it = std::next(std::begin(collection), rand() % std::size(collection));
			return it->first;
		}
		template <typename Collection>
		const typename Collection::value_type& RandomValue(const Collection& collection) {
			if (std::empty(collection)) throw std::runtime_error("Collection is empty!");
			auto it = std::next(std::begin(collection), rand() % std::size(collection));
			return it->second;
		}

		template <typename T = void> struct minus { using type = T; T operator()(const T& l, const T& r) const { return l - r; } };
		template <typename T = void> struct plus { using type = T; T operator()(const T& l, const T& r) const { return l + r; } };
		template <typename T = void> struct size { using type = T; u32 operator()(const T& t) const { return t.size(); } };

		template <typename T> T inline max(T a, T b) { return a > b ? a : b; }
		template <typename T> T inline min(T a, T b) { return a < b ? a : b; }
		inline u32 sub_safe(u32 a, u32 b) { return (b < a) * (a - b); }

		//template <typename Container>
		//const typename Container::value_type& Random(const Container& collection) {
		//  if (std::empty(collection)) throw std::runtime_error("Collection is empty!");
		//
		//  auto it = std::next(std::begin(collection), rand() % std::size(collection));
		//  return *it;
		//}
	}  // namespace util
}  // namespace pce