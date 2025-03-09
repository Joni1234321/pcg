#pragma once

#include <bitset>
#include <format>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include "0_engine/u_types.hpp"
#include "0_engine/u_util.hpp"

namespace pce {
template <typename T>concept TriviallyConstructible = std::is_trivially_constructible_v<T>;
template <typename T>concept DefaultConstructible = std::is_default_constructible_v<T>;

template <typename T, u32 N> using Array = std::array<T, N>;
template <typename T> using Span = std::span<T>;
template <typename T> using Stack = std::stack<T>;
template <class K, class V> using UnorderedMap = std::unordered_map<K, V>;
template <class T, class C = std::less<T>> using Set = std::set<T, C>;
template <class T, class C = std::less<T>> using Multiset = std::multiset<T, C>;
template <typename T, typename D> class UniquePointer {
    T* pointer;
    [[msvc::no_unique_address]][[no_unique_address]] D destructor { }; // man i love msvc
public:
    constexpr void Reset(T* new_pointer) noexcept {
        if (pointer != nullptr) { destructor(pointer); }
        pointer = new_pointer;
    }
    constexpr void Reset() noexcept { Reset(nullptr); }
    [[nodiscard]] constexpr T *Get() const noexcept { return pointer; }
    constexpr T *operator->() const noexcept { return pointer; }

    constexpr explicit UniquePointer(T* pointer) noexcept : pointer { pointer } { }
    UniquePointer(UniquePointer&& other) noexcept : pointer { std::exchange(other.pointer, nullptr) } { }
    UniquePointer(const UniquePointer& other) : pointer(other.pointer), destructor(other.destructor) { }
    UniquePointer& operator=(const UniquePointer& other) {
        if (this == &other) { return *this; }
        pointer = other.pointer;
        destructor = other.destructor;
        return *this;
    }
    UniquePointer& operator=(UniquePointer&& other) noexcept {
        if (this == &other) { return *this; }
        pointer = other.pointer;
        destructor = std::move(other.destructor);
        return *this;
    }
    constexpr ~UniquePointer() noexcept { if (pointer != nullptr) { destructor(pointer); } }
};
struct String {
    constexpr String() = default;
    constexpr String(const char character, const u32 count) : data(count, character, std::allocator<char>()) { } // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr String(std::string&& text) : data(std::move(text)) { }                                             // ReSharper disable once CppNonExplicitConvertingConstructor
    constexpr String(const char* text) : data(text, std::allocator<char>()) { }                                  // ReSharper disable once CppNonExplicitConvertingConstructor
    template <typename... Args> constexpr String(const char* text, Args... args) : data(std::vformat(text, std::make_format_args(args...))) { }

    operator const char*() const { return data.c_str(); }
    constexpr b8 operator ==(const String& other) const { return data == other.data; }
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
    [[nodiscard]] constexpr b8 empty() const { return data.empty(); }
    [[nodiscard]] constexpr const char *c_str() const { return data.c_str(); }
    [[nodiscard]] constexpr u32 size() const noexcept { return static_cast<u32>(data.size()); }
    constexpr void clear() noexcept { data.clear(); }

private:
    std::string data;
};

template <typename T> struct List {
    using value_type = typename std::vector<T>::value_type;
    using size_type = u32;

    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    using reverse_iterator = typename std::vector<T>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;

    constexpr List() : data() { }
    constexpr List(std::initializer_list<T> init_list) : data(std::move(init_list)) { }
    explicit constexpr List(u32 size) : data() { data.reserve(size); }
    explicit constexpr List(u32 size, const T& val) : data(size, val, std::allocator<T>()) { }
    template <class Iter> constexpr List(Iter first, Iter last) : data(first, last) { }

    // Generic collection converter  CREDIT goes the bot for this madness
    template <typename Container> explicit List(const Container& container) requires std::is_constructible_v<
        std::vector<T>, typename Container::iterator, typename Container::iterator> : data(container.begin(), container.end()) { }
    template <typename Container> explicit List(Container&& container) requires std::is_constructible_v<
        std::vector<T>, typename Container::iterator, typename Container::iterator> : data(std::make_move_iterator(container.begin()), std::make_move_iterator(container.end())) { }

    [[nodiscard]] constexpr u32 size() const noexcept { return static_cast<u32>(data.size()); }
    [[nodiscard]] constexpr u32 Size() const noexcept { return static_cast<u32>(data.size()); }

    operator Span<T>() { return Span<T>(*this); }

    constexpr const T& operator[](u32 pos) const { return data[pos]; }
    constexpr T& operator[](u32 pos) { return data[pos]; }

    template <typename... Args> constexpr T& EmplaceBack(Args&&... args) { return data.emplace_back(std::forward<Args>(args)...); }

    [[nodiscard]] constexpr iterator begin() noexcept { return data.begin(); }
    [[nodiscard]] constexpr iterator end() noexcept { return data.end(); }
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return data.begin(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return data.end(); }
    [[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return data.rbegin(); }
    [[nodiscard]] constexpr reverse_iterator rend() noexcept { return data.rend(); }
    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return data.rbegin(); }
    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return data.rend(); }

    [[nodiscard]] constexpr T& front() noexcept { return data.front(); }
    [[nodiscard]] constexpr T& back() noexcept { return data.back(); }
    [[nodiscard]] constexpr const T& front() const noexcept { return data.front(); }
    [[nodiscard]] constexpr const T& back() const noexcept { return data.back(); }

    [[nodiscard]] constexpr b8 empty() const noexcept { return data.empty(); }
    [[nodiscard]] constexpr b8 Contains(const T& value) const noexcept { return std::ranges::find(data, value, std::identity { }) != end(); }

    constexpr void clear() noexcept { data.clear(); }
    constexpr void pop_back() noexcept { data.pop_back(); }
    constexpr void resize(const u32 size) { data.resize(size); }
    constexpr void reserve(const u32 size) { data.reserve(size); }
    constexpr void erase_at(const u32 index) { data.erase(std::begin(data) + index); }
    constexpr u32 erase_value(const T& value) { return static_cast<u32>(std::erase(data, value)); }

    template <std::_Container_compatible_range<T> _Rng> constexpr void append_range(_Rng&& range) { data.append_range(range); }

    constexpr void push_back(const T& value) { data.push_back(value); }
    constexpr void push_back(T&& t) { data.push_back(std::move(t)); }

    void swap_back(u32 pos) {
        std::swap(data[pos], data.back());
        data.pop_back();
    }

    [[nodiscard]] u32 IndexOf(const T& t) const noexcept {
        u32 pos = 0;
        for (; pos < Size(); ++pos) { if (t == data[pos]) { break; } }
        return pos;
    }

    [[nodiscard]] List Limit(const u32 limit) const {
        if (empty()) { return List(); }
        auto first = begin();
        auto last = begin() + math::Min(limit, Size());
        return List(first, last);
    }

    template <typename To> [[nodiscard]] constexpr List<To> StaticCast() const {
        List<To> result { };
        result.reserve(Size());
        for (u32 i = 0U; i < Size(); i++) { result[i] = static_cast<To>(data[i]); }
        return result;
    }

    std::vector<T> data;
};

template <typename T, typename H = T> struct HandleList {
    using iterator = typename List<T>::iterator;
    using const_iterator = typename List<T>::const_iterator;

    constexpr HandleList() : data() { }
    constexpr HandleList(std::initializer_list<T> init_list) : data(std::move(init_list)) { }
    explicit constexpr HandleList(u32 size) : data(size) { }
    explicit constexpr HandleList(u32 size, const T& val) : data(size, val) { }

    template <typename... Args> [[nodiscard]] constexpr Handle<H> EmplaceBack(Args&&... args) {
        data.EmplaceBack(std::forward<Args>(args)...);
        return Handle<H> { offset_handle.id + data.size() - 1U };
    }
    [[nodiscard]] constexpr Handle<H> PushBack(const T& t) {
        data.push_back(t);
        return Handle<H> { offset_handle.id + data.size() - 1U };
    }
    [[nodiscard]] constexpr Handle<H> PushBack(T&& t) {
        data.push_back(std::move(t));
        return Handle<H> { offset_handle.id + data.size() - 1U };
    }
    [[nodiscard]] constexpr b8 empty() const noexcept { return data.empty(); }
    [[nodiscard]] constexpr u32 size() const noexcept { return data.size(); }

    constexpr void clear() {
        offset_handle = Handle<H> { offset_handle.id + size() };
        data.clear();
    }

    [[nodiscard]] constexpr const T& operator[](const Handle<H> handle) const { return data[HandleToIndex(handle)]; }
    [[nodiscard]] constexpr T& operator[](const Handle<H> handle) { return data[HandleToIndex(handle)]; }

    [[nodiscard]] constexpr iterator begin() noexcept { return data.begin(); }
    [[nodiscard]] constexpr iterator end() noexcept { return data.end(); }
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return data.begin(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return data.end(); }

    [[nodiscard]] constexpr T& front() noexcept { return data.front(); }
    [[nodiscard]] constexpr T& back() noexcept { return data.back(); }
    [[nodiscard]] constexpr const T& front() const noexcept { return data.front(); }
    [[nodiscard]] constexpr const T& back() const noexcept { return data.back(); }

    [[nodiscard]] constexpr u32 HandleToIndex(const Handle<H> handle) const noexcept {
        STL_ASSERT(ValidHandle(handle), "handle invalidated");
        return handle.id - offset_handle.id;
    }
    [[nodiscard]] constexpr Handle<H> IndexToHandle(const u32 index) const noexcept {
        STL_ASSERT(index < size(), "index bigger than size");
        return ::Handle<H> { offset_handle.id + index };
    }
    [[nodiscard]] constexpr Handle<H> IteratorToHandle(const_iterator iterator) const noexcept {
        STL_ASSERT(iterator > end(), "iterator out of range");
        return ::Handle<H> { offset_handle.id + static_cast<u32>(std::distance(begin(), iterator)) };
    }
    [[nodiscard]] constexpr b8 ValidHandle(const Handle<H> handle) const noexcept { return handle.id - offset_handle.id < size(); }
    [[nodiscard]] constexpr Handle<H> FirstHandle() const noexcept { return offset_handle; }
    Handle<H> offset_handle { 0U };

    constexpr void swap_back(const Handle<H> handle) {
        // std::swap(particle, emitter.particles.back()); emitter.particles.data.data.pop_back();
    }

    List<T> data;
};
template <typename K, typename V> class FlatMap {
    List<K> keys { };
    List<V> values { };

public:
    using ValueType = V;
    using Pointer = V*;
    using ConstPointer = const V*;
    using ValueReference = V&;
    using ConstReference = const V&;
    using SizeType = u32;

    constexpr FlatMap() = default;
    explicit constexpr FlatMap(const u32 initial_size) : keys(initial_size), values(initial_size) { }
    constexpr void PushBack(const K& key, const V& value) {
        keys.push_back(key);
        values.push_back(value);
    }
    constexpr void Erase(const K& key) {
        u32 index = keys.IndexOf(key);
        keys.erase_at(index);
        values.erase_at(index);
    }
    constexpr void PushBack(const K& key, V&& value) {
        keys.push_back(key);
        values.push_back(value);
    }

    template <class... Args> constexpr void EmplaceBack(const K& key, Args&&... args) {
        keys.EmplaceBack(key);
        values.EmplaceBack(std::forward<Args>(args)...);
    }

    constexpr void Clear() {
        keys.clear();
        values.clear();
    }

    [[nodiscard]] constexpr b8 HasKey(const K& key) const { return std::ranges::find(keys, key) != keys.end(); }
    [[nodiscard]] constexpr V& operator[](const K& key) {
        const u32 pos = keys.IndexOf(key);
        return values[pos];
    }

    [[nodiscard]] constexpr const List<K>& Keys() { return keys; }
    [[nodiscard]] constexpr const List<V>& Values() { return values; }
};

template <typename T> struct Queue : List<T> {
    void RemoveAt(u32 pos) { this->data.erase(this->data.begin() + pos); }
    [[nodiscard]] constexpr T Front() { return this->data.front(); }
    void Pop() { this->data.erase(this->data.begin()); }
    Span<T> LastElementsToSpan(u32 elements) { return Span<T>(this->data.end() - elements, this->data.end()); }
};

template <typename T> using Component = List<T>;

struct Entities : List<Entity> {
    constexpr Entities() = default;
    constexpr ~Entities() = default;
    Entities(const Entities&) = delete;
    Entities& operator=(const Entities&) = delete;
    Entities(Entities&&) = delete;
    Entities& operator=(Entities&&) = delete;
};

struct Parent : List<Entity> {
    constexpr Parent() = default;
    constexpr ~Parent() = default;
    Parent(const Parent&) = delete;
    Parent& operator=(const Parent&) = delete;
    Parent(Parent&&) = delete;
    Parent& operator=(Parent&&) = delete;
};

template <class T> struct Pool {
    List<T> items;
    u32 deleted { 0U };
    constexpr Pool(u32 default_size) : items { default_size } { }
    void ApplyErase() {
        if (deleted == 0U) { return; }
        items.resize(items.size() - deleted);
        deleted = 0U;
    }
    void SwapBackErase(T& item) {
        T& back = *(items.end() - ++deleted);
        if (std::addressof(item) != std::addressof(back)) { std::swap(item, back); }
    }
};
} // namespace pce
