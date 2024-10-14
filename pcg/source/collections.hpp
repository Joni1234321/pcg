#pragma once

#include "types.hpp"

#include <span>
#include <vector>

namespace pce {
template <typename T> using Span = std::span<T>;
template <typename T> using List = std::vector<T>;
template <typename T> using Component = std::vector<T>;
template<typename T>
struct Array : private std::span<T> {
	Array(u32 n) : std::span<T>(new T[n](), n) {
		// std::printf("CONSTRUCTING %s \n", typeid(T).name());
	}
	Array(const Array<T> &other) = delete;
	//Array(const Array<T> &other) : std::span<T>(other) { std::printf("COPYING %s\n", typeid(T).name()); }
	Array(Array<T> &&other) noexcept {
		std::printf("MOVING %s\n", typeid(T).name());
	}
	~Array() {
		std::printf("DELETING %s\n", typeid(T).name());
		delete[] this->data();
	}
	// Move assignment operator: Transfer ownership during assignment
	Array<T> &operator=(Array<T> &&other) noexcept {
		if (this != &other) {
			// Clean up existing resource
			delete[] this->data();

			// Take ownership of other's resources
			std::span<T>::operator=(std::span<T>(other.data(), other.size()));

			std::printf("MOVE ASSIGNING %s\n", typeid(T).name());
			other.reset();  // Nullify the other object
		}
		return *this;
	}

	using std::span<T>::operator[];
	using std::span<T>::begin;
	using std::span<T>::end;
	using std::span<T>::size;
	
	void Clear () {}

};

struct Parents : private List<Entity> {
	Parents() : List<Entity>() {}
	using List<Entity>::operator[];
	using List<Entity>::emplace_back;
	using List<Entity>::begin;
	using List<Entity>::end;
	using List<Entity>::size;
};
struct Entities : private List<Entity> {
	Entities() : List<Entity>() {}
	using List<Entity>::operator[];
	using List<Entity>::emplace_back;
	using List<Entity>::begin;
	using List<Entity>::end;
	using List<Entity>::size;
};


constexpr void split(Array<List<Entity>> &re, const List<Entity> &entities, const Parents &parents) {
	for (const Entity &entity : entities) {
		const Entity &parent = parents[entity.index];
		re[parent.index].emplace_back(entity);
	}
}
inline Array<List<Entity>> split(const List<Entity> &entities, const Parents &parents, const u32 n) {
	Array<List<Entity>> re(n);
	split(re, entities, parents);
	return re;
}
// assumes all entities are tightly packed and linked to parents
constexpr void split(Array<List<Entity>> &re, const Parents &parents) {
	for (Entity ent = 0; ent.index < parents.size(); ent.index++) {
		const Entity &parent = parents[ent.index];
		re[parent.index].emplace_back(ent);
	}
}
inline Array<List<Entity>> split(const Parents &parents, const u32 n) {
	Array<List<Entity>> re(n);
	split(re, parents);
	return re;
}
}