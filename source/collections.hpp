#pragma once

#include <span>
#include <vector>

#include "types.hpp"
#include "util.hpp"

namespace pce {
template <typename T> using Span = std::span<T>;

template <typename T>
struct Array : private std::span<T> {
	Array(u32 n) : std::span<T>(new T[n](), n) {
		std::printf("CONSTRUCTING %s \n", typeid(T).name());
	}
	Array(const Array<T> &other) = delete;
	Array(Array<T> &&other) noexcept {
		std::printf("MOVING %s\n", typeid(T).name());
	}
	~Array() {
		std::printf("DELETING %s\n", typeid(T).name());
		delete[] this->data();
	}
	Array<T> &operator=(Array<T> &&other) noexcept {
		if (this != &other) {
			delete[] this->data();

			std::span<T>::operator=(std::span<T>(other.data(), other.size()));

			std::printf("MOVE ASSIGNING %s\n", typeid(T).name());
			other.reset();
		}
		return *this;
	}

	using std::span<T>::operator[];
	using std::span<T>::begin;
	using std::span<T>::end;
	using std::span<T>::size;
};

_EXPORT_STD template <bool _Test, class _Ty = void>
struct enable_if {}; // no member "type" when !_Test

template <class _Ty>
struct enable_if<true, _Ty> { // type is _Ty for _Test
	using type = _Ty;
};

_EXPORT_STD template <bool _Test, class _Ty = void>
using enable_if_t = typename enable_if<_Test, _Ty>::type;

template <typename T> struct List {

	std::vector<T> data;

	using Iter = std::vector<T>::iterator;
	using CIter = std::vector<T>::const_iterator;
	using value_type = std::vector<T>::value_type;

	constexpr List() : data() {}
	constexpr ~List() {}

	template <class _Iter>
	constexpr List(_Iter first, _Iter last) : data(first, last) {}

	constexpr u32 size() const { return data.size(); }

	operator Span<T>() { return Span<T>(*this); }

	// Generic collection converter  CREDIT goes the bot for this madness
	template <typename Container, typename = std::enable_if_t<std::is_constructible<std::vector<T>, typename Container::iterator, typename Container::iterator>::value>>
	List(const Container &container) : data(container.begin(), container.end()) {}
	template <typename Container, typename = std::enable_if_t<std::is_constructible<std::vector<T>, typename Container::iterator, typename Container::iterator>::value>>
	List(Container &&container) : data(std::make_move_iterator(container.begin()), std::make_move_iterator(container.end())) {}


	constexpr const T &operator[](u32 i) const { return data[i]; }
	constexpr T &operator[](u32 i) { return data[i]; }

	template <class... Args> T &emplace_back(Args&&... args) { return data.emplace_back(std::forward<Args>(args)...); }

	constexpr Iter begin() { return data.begin(); }
	constexpr Iter end() { return data.end(); }
	constexpr T &front() { return data.front(); }
	constexpr T &back() { return data.back(); }

	constexpr CIter begin() const { return data.begin(); }
	constexpr CIter end() const { return data.end(); }
	constexpr const T &front() const { return data.front(); }
	constexpr const T &back() const { return data.back(); }

	constexpr void pop_back() { data.pop_back(); }
	constexpr void resize(u32 size) { data.resize(size); }

	constexpr bool empty() const { return data.empty(); }
	constexpr void push_back(T &t) { data.push_back(t); }
	constexpr void push_back(T &&t) { data.push_back(std::move(t)); }

	void swap_back(u32 i) {
		std::swap(data[i], data.back());
		data.pop_back();
	}

	List<T> Limit(u32 n) const {
		if (empty()) return List<T>();
		auto first = begin();
		auto last = begin() + util::min(n, size());
		List<T> l(first, last);
		return l;
	}
};


template <typename T> struct Queue : public List<T> {
	void pop() { this->data.erase(this->data.begin()); }
};

template <typename T> using Component = List<T>;

struct Parent : private List<Entity> {
	constexpr Parent() : List<Entity>() {}
	constexpr ~Parent() { List<Entity>::~List(); }
	using List<Entity>::operator[];
	using List<Entity>::emplace_back;
	using List<Entity>::begin;
	using List<Entity>::back;
	using List<Entity>::end;
	using List<Entity>::size;
	using List<Entity>::pop_back;
	using List<Entity>::swap_back;
	using List<Entity>::value_type;
};
struct Entities : private List<Entity> {
	constexpr Entities() : List<Entity>() {}
	using List<Entity>::operator[];
	using List<Entity>::emplace_back;
	using List<Entity>::begin;
	using List<Entity>::end;
	using List<Entity>::size;
	using List<Entity>::pop_back;
	using List<Entity>::swap_back;
	using List<Entity>::value_type;
};

constexpr void split(Array<List<Entity>> &re, const List<Entity> &entities, const Parent &parents) {
	for (const Entity &entity : entities) {
		const Entity &parent = parents[entity.index];
		re[parent.index].emplace_back(entity);
	}
}
inline Array<List<Entity>> split(const List<Entity> &entities,
	const Parent &parents, const u32 n) {
	Array<List<Entity>> re(n);
	split(re, entities, parents);
	return re;
}
// assumes all entities are tightly packed and linked to parents
constexpr void split(Array<List<Entity>> &re, const Parent &parents) {
	for (Entity e : parents) {
		const Entity &parent = parents[e.index];
		re[parent.index].emplace_back(e);
	}
}
inline Array<List<Entity>> split(const Parent &parents, const u32 n) {
	Array<List<Entity>> re(n);
	split(re, parents);
	return re;
}
}  // namespace pce