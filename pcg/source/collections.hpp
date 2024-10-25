#pragma once

#include <span>
#include <vector>

#include "types.hpp"

namespace pce {
template <typename T>
using Span = std::span<T>;
template <typename T>
using List = std::vector<T>;
template <typename T>
using Component = std::vector<T>;
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

struct Parent : private List<Entity> {
  constexpr Parent() : List<Entity>() {}
  constexpr ~Parent() { List<Entity>::~vector(); }
  using List<Entity>::operator[];
  using List<Entity>::emplace_back;
  using List<Entity>::begin;
  using List<Entity>::back;
  using List<Entity>::end;
  using List<Entity>::size;
  using List<Entity>::pop_back;
};
struct Entities : private List<Entity> {
  constexpr Entities() : List<Entity>() {}
  using List<Entity>::operator[];
  using List<Entity>::emplace_back;
  using List<Entity>::begin;
  using List<Entity>::end;
  using List<Entity>::size;
  using List<Entity>::pop_back;
};

constexpr void split(Array<List<Entity>> &re, const List<Entity> &entities,
                     const Parent &parents) {
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
  for (Entity ent = 0; ent.index < parents.size(); ent.index++) {
    const Entity &parent = parents[ent.index];
    re[parent.index].emplace_back(ent);
  }
}
inline Array<List<Entity>> split(const Parent &parents, const u32 n) {
  Array<List<Entity>> re(n);
  split(re, parents);
  return re;
}
}  // namespace pce