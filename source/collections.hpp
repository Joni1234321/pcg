#pragma once

#include <format>
#include <span>
#include <utility>
#include <vector>

#include "types.hpp"
#include "util.hpp"

namespace pce {
template <typename T> using Span = std::span<T>;

struct String {
    constexpr String() = default;
    constexpr String(const char character, const u32 count) : data(count, character, std::allocator<char>()) { }
    constexpr String(std::string&& text) : data(std::move(text)) { }            // NOLINT(*-explicit-constructor, *-explicit-conversions)
    constexpr String(const char* text) : data(text, std::allocator<char>()) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)
    template <typename... Args> constexpr String(const char* text, Args... args) : data(std::vformat(text, std::make_format_args(args...))) { }

    operator const char*() const { return data.c_str(); }
    constexpr String& operator +=(const String& other) {
        data += other.data;
        return *this;
    }
    constexpr String& operator +=(const String&& other) {
        data += other.data;
        return *this;
    }

    constexpr void Add(const String& other) { data += other.data; }
    constexpr void Add(const String&& other) { data += other.data; }
    [[nodiscard]] constexpr b8 Empty() const { return data.empty(); }
    [[nodiscard]] constexpr const char *CString() const { return data.c_str(); }
    [[nodiscard]] u32 size() const { return static_cast<u32>(data.size()); }
    constexpr void Clear() { data.clear(); }

private:
    std::string data;
};
template <typename T> struct Array : private std::span<T> {
    explicit Array(u32 n) : std::span<T>(new T[n](), n) { (void)std::printf("CONSTRUCTING %s \n", typeid(T).name()); }
    Array(const Array<T>& other) = delete;
    Array(Array<T>&& other) noexcept { (void)std::printf("MOVING %s\n", typeid(T).name()); }
    ~Array() {
        (void)std::printf("DELETING %s\n", typeid(T).name());
        delete[] this->data();
    }
    Array<T>& operator=(Array<T>&& other) noexcept {
        if (this != &other) {
            delete[] this->data();

            std::span<T>::operator=(std::span<T>(other.data(), other.size()));

            (void)std::printf("MOVE ASSIGNING %s\n", typeid(T).name());
            other.reset();
        }
        return *this;
    }

    using std::span<T>::operator[];
    using std::span<T>::begin;
    using std::span<T>::end;
    using std::span<T>::size;
};

template <typename T> struct List {
    using Iter = typename std::vector<T>::iterator;
    using CIter = typename std::vector<T>::const_iterator;
    using value_type = typename std::vector<T>::value_type;

    constexpr List() : data() { }
    constexpr List(std::initializer_list<T> init_list) : data(init_list) { }
    explicit constexpr List(u32 size) : data(size, std::allocator<T>()) { }
    constexpr ~List() = default;

    template <class Iter> constexpr List(Iter first, Iter last) : data(first, last) { }

    [[nodiscard]] constexpr u32 Size() const { return data.size(); }

    operator Span<T>() { return Span<T>(*this); }

    // Generic collection converter  CREDIT goes the bot for this madness
    template <typename Container> explicit List(const Container& container) requires std::is_constructible_v<
        std::vector<T>, typename Container::iterator, typename Container::iterator> : data(container.begin(), container.end()) { }
    template <typename Container> explicit List(Container&& container) requires std::is_constructible_v<
        std::vector<T>, typename Container::iterator, typename Container::iterator> : data(std::make_move_iterator(container.begin()), std::make_move_iterator(container.end())) { }

    constexpr const T& operator[](u32 pos) const { return data[pos]; }
    constexpr T& operator[](u32 pos) { return data[pos]; }

    template <class... Args> T& EmplaceBack(Args&&... args) { return data.emplace_back(std::forward<Args>(args)...); }

    constexpr Iter begin() { return data.begin(); }
    constexpr Iter end() { return data.end(); }
    constexpr T& Front() { return data.front(); }
    constexpr T& Back() { return data.back(); }

    [[nodiscard]] constexpr CIter begin() const { return data.begin(); }
    [[nodiscard]] constexpr CIter end() const { return data.end(); }
    [[nodiscard]] constexpr const T& Front() const { return data.front(); }
    [[nodiscard]] constexpr const T& Back() const { return data.back(); }

    constexpr void PopBack() { data.pop_back(); }
    constexpr void Resize(const u32 size) { data.resize(size); }
    constexpr void Reserve(const u32 size) { data.reserve(size); }
    constexpr void push_back(const T& value) { data.push_back(value); }

    [[nodiscard]] constexpr b8 Empty() const { return data.empty(); }
    constexpr void PushBack(T& t) { data.push_back(t); }
    constexpr void PushBack(T&& t) { data.push_back(std::move(t)); }

    void SwapBack(u32 pos) {
        std::swap(data[pos], data.back());
        data.pop_back();
    }

    [[nodiscard]] List<T> Limit(const u32 limit) const {
        if (Empty()) { return List<T>(); }
        auto first = begin();
        auto last = begin() + math::Min(limit, Size());
        return List<T>(first, last);
    }

    template <typename To> [[nodiscard]] constexpr List<To> StaticCast() const {
        List<To> result { };
        result.Reserve(Size());
        for (u32 i = 0U; i < Size(); i++) { result[i] = static_cast<To>(data[i]); }
        return result;
    }

protected:
    std::vector<T> data;
};

template <typename T> struct Queue : public List<T> {
    void RemoveAt(u32 pos) { this->data.erase(this->data.begin() + pos); }
    void Pop() { this->data.erase(this->data.begin()); }
};

template <typename T> using Component = List<T>;

struct Entities : private List<Entity> {
    constexpr Entities() = default;
    constexpr ~Entities() { List<Entity>::~List(); }
    Entities(const Entities&) = delete;
    Entities& operator=(const Entities&) = delete;
    Entities(Entities&&) = delete;
    Entities& operator=(Entities&&) = delete;
    using List<Entity>::operator[];
    using List<Entity>::EmplaceBack;
    using List<Entity>::begin;
    using List<Entity>::end;
    using List<Entity>::Size;
    using List<Entity>::PopBack;
    using List<Entity>::SwapBack;
    using List<Entity>::value_type;
};

struct Parent : private List<Entity> {
    constexpr Parent() = default;
    constexpr ~Parent() { List<Entity>::~List(); }
    Parent(const Parent&) = delete;
    Parent& operator=(const Parent&) = delete;
    Parent(Parent&&) = delete;
    Parent& operator=(Parent&&) = delete;
    using List<Entity>::operator[];
    using List<Entity>::EmplaceBack;
    using List<Entity>::begin;
    using List<Entity>::Back;
    using List<Entity>::end;
    using List<Entity>::Size;
    using List<Entity>::PopBack;
    using List<Entity>::SwapBack;
    using List<Entity>::value_type;
};

constexpr void split(Array<List<Entity>>& ret, const List<Entity>& entities, const Parent& parents) {
    for (const Entity& entity : entities) {
        const Entity& parent = parents[static_cast<u32>(entity)];
        (void)ret[parent].EmplaceBack(entity);
    }
}
inline Array<List<Entity>> split(const List<Entity>& entities, const Parent& parents, const u32 n) {
    Array<List<Entity>> ret(n);
    split(ret, entities, parents);
    return ret;
}
// assumes all entities are tightly packed and linked to parents
constexpr void split(Array<List<Entity>>& ret, const Parent& parents) {
    for (Entity entity : parents) {
        const Entity& parent = parents[entity];
        (void)ret[parent].EmplaceBack(entity);
    }
}
inline Array<List<Entity>> split(const Parent& parents, const u32 n) {
    Array<List<Entity>> ret(n);
    split(ret, parents);
    return ret;
}
} // namespace pce
