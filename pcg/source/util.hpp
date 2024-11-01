#pragma once

#include <vector>

#include "collections.hpp"
#include "types.hpp"
#include "unordered_map"
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
		constexpr void SwapPop(Parent v, u32 i) {
			std::swap(v[i], v.back());
			v.pop_back();
		}

		template <typename T>
		constexpr Array<u32> get_inner_sizes(const List<List<T>>& v) {
			Array<u32> sizes(v.size());
			for (int i = 0; i < sizes.length; i++) sizes[i] = v[i].size();
			return sizes;
		}
		template <typename T>
		constexpr Array<u32> get_inner_sizes(const Array<List<T>>& v) {
			Array<u32> sizes(v.size());
			for (int i = 0; i < sizes.size(); i++) sizes[i] = v[i].size();
			return sizes;
		}
		template <typename T>
		constexpr void clear_inner(List<List<T>>& v) {
			for (List<T>& vi : v) vi.clear();
		}
		template <typename T>
		constexpr void clear_inner(Array<List<T>>& v) {
			for (List<T>& vi : v) vi.clear();
		}
		constexpr u32 sub_safe(u32 a, u32 b) { return (b < a) * (a - b); }

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
		template <typename T = void> struct size { using type = T; u32 operator()(const T& t) const { return std::size(t); } };



		//template <typename Container>
		//const typename Container::value_type& Random(const Container& collection) {
		//  if (std::empty(collection)) throw std::runtime_error("Collection is empty!");
		//
		//  auto it = std::next(std::begin(collection), rand() % std::size(collection));
		//  return *it;
		//}
	}  // namespace util
}  // namespace pce