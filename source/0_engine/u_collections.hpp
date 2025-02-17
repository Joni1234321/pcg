#pragma once

#include <chrono>
#include <format>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include "u_types.hpp"
#include "u_util.hpp"

namespace pce {
template <typename T, u32 N> using Array = std::array<T, N>;
template <typename T> using Span = std::span<T>;
template <typename T> using Stack = std::stack<T>;
template <class K, class V> using UnorderedMap = std::unordered_map<K, V>;
template <class T, class C = std::less<T>> using Set = std::set<T, C>;
template <class T, class C = std::less<T>> using Multiset = std::multiset<T, C>;

using namespace std::chrono_literals;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Duration = std::chrono::duration;
using Seconds = std::chrono::seconds;
using Milliseconds = std::chrono::milliseconds;
using Nanoseconds = std::chrono::nanoseconds;
[[nodiscard]] inline TimePoint TimeNow() noexcept { return std::chrono::high_resolution_clock::now(); };
// template <typename T> [[nodiscard]] Duration DurationCast (Duration duration) { return std::chrono::duration_cast<T>(duration); }


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

    constexpr explicit UniquePointer(T* pointer) noexcept : pointer { pointer } { }
    constexpr explicit UniquePointer(UniquePointer&& other) noexcept : pointer { std::exchange(other.pointer, nullptr) } { }
    constexpr ~UniquePointer() noexcept { if (pointer != nullptr) { destructor(pointer); } }
};
struct String {
    constexpr String() = default;
    constexpr String(const char character, const u32 count) : data(count, character, std::allocator<char>()) { }
    constexpr String(std::string&& text) : data(std::move(text)) { }            // NOLINT(*-explicit-constructor, *-explicit-conversions)
    constexpr String(const char* text) : data(text, std::allocator<char>()) { } // NOLINT(*-explicit-constructor, *-explicit-conversions)
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
    [[nodiscard]] constexpr b8 Empty() const { return data.empty(); }
    [[nodiscard]] constexpr const char *CString() const { return data.c_str(); }
    [[nodiscard]] u32 Size() const { return static_cast<u32>(data.size()); }
    [[nodiscard]] u32 size() const { return static_cast<u32>(data.size()); }
    constexpr void Clear() { data.clear(); }

private:
    std::string data;
};

template <typename T> struct List {
    using Iter = typename std::vector<T>::iterator;
    using CIter = typename std::vector<T>::const_iterator;
    using value_type = typename std::vector<T>::value_type;

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

    [[nodiscard]] constexpr u32 Size() const { return data.size(); }

    operator Span<T>() { return Span<T>(*this); }

    constexpr const T& operator[](u32 pos) const { return data[pos]; }
    constexpr T& operator[](u32 pos) { return data[pos]; }

    template <typename... Args> constexpr T& EmplaceBack(Args&&... args) { return data.emplace_back(std::forward<Args>(args)...); }

    [[nodiscard]] constexpr Iter begin() { return data.begin(); }
    [[nodiscard]] constexpr Iter end() { return data.end(); }
    [[nodiscard]] constexpr T& Front() { return data.front(); }
    [[nodiscard]] constexpr T& Back() { return data.back(); }

    [[nodiscard]] constexpr CIter begin() const { return data.begin(); }
    [[nodiscard]] constexpr CIter end() const { return data.end(); }
    [[nodiscard]] constexpr const T& Front() const { return data.front(); }
    [[nodiscard]] constexpr const T& Back() const { return data.back(); }

    constexpr void Clear() { data.clear(); }
    constexpr void PopBack() { data.pop_back(); }
    constexpr void Resize(const u32 size) { data.resize(size); }
    constexpr void Reserve(const u32 size) { data.reserve(size); }
    // ReSharper disable once CppInconsistentNaming
    constexpr void push_back(const T& value) { data.push_back(value); }
    template <std::_Container_compatible_range<T> _Rng> constexpr void AppendRange(_Rng&& range) { data.append_range(range); }

    [[nodiscard]] constexpr b8 Empty() const { return data.empty(); }
    constexpr void PushBack(const T& t) { data.push_back(t); }
    constexpr void PushBack(T&& t) { data.push_back(std::move(t)); }

    void SwapBack(u32 pos) {
        std::swap(data[pos], data.back());
        data.pop_back();
    }

    u32 IndexOf(const T& t) const {
        u32 pos = 0;
        for (; pos < Size(); ++pos) { if (t == data[pos]) { break; }; }
        return pos;
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

template <typename T, typename H = Handle<T>> struct HandleList {
    using Iter = typename List<T>::Iter;
    using CIter = typename List<T>::CIter;
    using Handle = H;

    constexpr HandleList() : data() { }
    constexpr HandleList(std::initializer_list<T> init_list) : data(std::move(init_list)) { }
    explicit constexpr HandleList(u32 size) : data(size) { }
    explicit constexpr HandleList(u32 size, const T& val) : data(size, val) { }

    template <typename... Args> [[nodiscard]] constexpr Handle EmplaceBack(Args&&... args) {
        data.EmplaceBack(std::forward<Args>(args)...);
        return Handle { offset_handle.id + data.Size() - 1U };
    }
    [[nodiscard]] constexpr Handle PushBack(const T& t) {
        data.PushBack(t);
        return Handle { offset_handle.id + data.Size() - 1U };
    }
    [[nodiscard]] constexpr Handle PushBack(T&& t) {
        data.PushBack(std::move(t));
        return Handle { offset_handle.id + data.Size() - 1U };
    }
    [[nodiscard]] constexpr b8 Empty() const { return data.Empty(); }
    [[nodiscard]] constexpr u32 Size() const { return data.Size(); }

    constexpr void Clear() {
        offset_handle = Handle { offset_handle.id + Size() };
        data.Clear();
    }

    [[nodiscard]] constexpr const T& operator[](const Handle handle) const { return data[HandleToIndex(handle)]; }
    [[nodiscard]] constexpr T& operator[](const Handle handle) { return data[HandleToIndex(handle)]; }

    [[nodiscard]] constexpr Handle First() const { return offset_handle; }

    [[nodiscard]] constexpr Iter begin() { return data.begin(); }
    [[nodiscard]] constexpr Iter end() { return data.end(); }
    [[nodiscard]] constexpr T& Front() { return data.Front(); }
    [[nodiscard]] constexpr T& Back() { return data.Back(); }

    [[nodiscard]] constexpr CIter begin() const { return data.begin(); }
    [[nodiscard]] constexpr CIter end() const { return data.end(); }
    [[nodiscard]] constexpr const T& Front() const { return data.Front(); }
    [[nodiscard]] constexpr const T& Back() const { return data.Back(); }

    [[nodiscard]] constexpr u32 HandleToIndex(const Handle handle) const {
        assert(ValidHandle(handle));
        return handle.id - offset_handle.id;
    }
    [[nodiscard]] constexpr b8 ValidHandle(const Handle handle) const { return (handle.id - offset_handle.id) < Size(); }
    Handle offset_handle { 0U };

private:
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
    constexpr void PushBack(const K& key, V&& value) {
        keys.PushBack(key);
        values.PushBack(value);
    };

    template <class... Args> constexpr void EmplaceBack(const K& key, Args&&... args) {
        keys.EmplaceBack(key);
        values.EmplaceBack(std::forward<Args>(args)...);
    };

    constexpr void Clear() {
        keys.Clear();
        values.Clear();
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
} // namespace pce
