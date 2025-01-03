#pragma once

#include <format>
#include <span>
#include <utility>
#include <vector>
#include <filesystem>

#include "u_types.hpp"
#include "u_util.hpp"

namespace pce {
template <typename T> using Span = std::span<T>;
using AbsolutePath = std::filesystem::path;
using RelativePath = std::filesystem::path;
using AssetPath = std::filesystem::path;

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

template <typename T> struct List {
    using Iter = typename std::vector<T>::iterator;
    using CIter = typename std::vector<T>::const_iterator;
    using value_type = typename std::vector<T>::value_type;

    constexpr List() : data() { }
    constexpr List(std::initializer_list<T> init_list) : data(init_list) { }
    explicit constexpr List(u32 size) : data() { data.reserve(size); }
    explicit constexpr List(u32 size, const T& val) : data(size, val, std::allocator<T>()) { }
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
    // ReSharper disable once CppInconsistentNaming
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
