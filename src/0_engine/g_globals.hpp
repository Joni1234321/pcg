#pragma once
#include <typeindex>

#include "0_engine/u_collections.hpp"
#include "0_engine/u_types.hpp"

namespace pce {
// Stored somewhere and deletable
struct DataWrapper {
    void* ptr;
    template <class T> [[nodiscard]] constexpr HandleList<T>& Get() { return *static_cast<HandleList<T>*>(ptr); }
};
template <class T> [[nodiscard]] constexpr DataWrapper MakeWrapper() {
    constexpr u32 DEFAULT_SIZE = 128U;
    return DataWrapper { .ptr = new HandleList<T> { DEFAULT_SIZE } };
}
struct Data {
    using Key = std::type_index;
    using Database = UnorderedMap<Key, DataWrapper>;
    Database tables;

    template <typename T> [[nodiscard]] constexpr HandleList<T>& Get() {
        const Key key { typeid(T) };
        DataWrapper& table = tables.try_emplace(key, MakeWrapper<T>()).first->second;
        return table.Get<T>();
    }
    template <typename T> [[nodiscard]] constexpr T& operator[](Handle<T> handle) { return Get<T>()[handle]; }
    template <class T, typename... Args> constexpr Handle<T> Create(Args&&... args) { return Get<T>().EmplaceBack(std::forward<Args>(args)...); }
};

// Instant no reset
template <class T> struct SingletonHandleList {
    static HandleList<T> data;
};
template <class T> HandleList<T> SingletonHandleList<T>::data { 128U };
struct GlobalDataInstantNoReset {
    template <class T> [[nodiscard]] constexpr HandleList<T>& Get() { return SingletonHandleList<T>::data; }
    template <class T> [[nodiscard]] constexpr T& operator[](Handle<T> handle) { return Get<T>()[handle]; }
    template <class T> [[nodiscard]] constexpr Handle<T> Create(const T& copy) { return Get<T>().EmplaceBack(std::forward<T>(copy)); }
    template <class T, typename... Args> [[nodiscard]] constexpr Handle<T> Create(Args&&... args) { return Get<T>().EmplaceBack(std::forward<Args>(args)...); }
};
template <DefaultConstructible T> struct SingletonNaiveHelper {
    static T data;
};
template <DefaultConstructible T> T SingletonNaiveHelper<T>::data { };
struct SingletonNaive2 {
    template <DefaultConstructible T> [[nodiscard]] static constexpr T& Get() { return SingletonNaiveHelper<T>::data; }
};
struct Singleton {
    template <DefaultConstructible T> [[nodiscard]] static constexpr T& Get() {
        static T instance;
        return instance;
    }
};

inline GlobalDataInstantNoReset data { };
inline Singleton singleton { };
} // namespace pce
