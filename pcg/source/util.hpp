#pragma once

#include "collections.hpp"
#include "types.hpp"

#include <vector>

namespace pce {
namespace util {
template <typename T>
constexpr void SwapPop(std::vector<T> &v, const std::vector<Entity> &entities) {
	const size_t n = entities.size();
	for (u32 i = 0; i < n; i++) {
		const u32 idx = entities[n - i - 1].index;
		std::swap(v[idx], v.back());
		v.pop_back();
	}
}
template <typename T>
constexpr void SwapPop(std::vector<T> &v, u32 i) {
	std::swap(v[i], v.back());
	v.pop_back();
}

template <typename T>
constexpr Array<u32> get_inner_sizes(const List<List<T>> &v) {
	Array<u32> sizes(v.size());
	for (int i = 0; i < sizes.length; i++) sizes[i] = v[i].size();
	return sizes;
}
template <typename T>
constexpr Array<u32> get_inner_sizes(const Array<List<T>> &v) {
	Array<u32> sizes(v.size());
	for (int i = 0; i < sizes.size(); i++) sizes[i] = v[i].size();
	return sizes;
}
template<typename T>
constexpr void clear_inner(List<List<T>> &v) { for (List<T> &vi : v) vi.clear(); }
template <typename T>
constexpr void clear_inner(Array<List<T>> &v) { for (List<T> &vi : v) vi.clear(); }
constexpr u32 sub_safe(u32 a, u32 b) {
	return (b < a) * (a - b);
}
//const size_t ARENA_SIZE = 16384;
//class Arena {
//public:
//    Arena()
//    {
//        ptr_ = malloc(ARENA_SIZE);
//    }

//private:
//    void* ptr_;
//};
}
}