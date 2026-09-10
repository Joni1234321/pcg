module;

#include <cassert>

export module pce.collections.hive;
import std;
import pce.std;
import pce.collections;
export namespace hex {
template <class T, u32 BLOCK_SIZE = 64U> class Hive {
    struct Slot {
        alignas(T) std::byte bytes[sizeof(T)];
        Slot* next_free { nullptr };
        b8 occupied { false };
        [[nodiscard]] T& Value() { return *std::launder(reinterpret_cast<T*>(bytes)); }
    };
    List<UniquePointer<Array<Slot, BLOCK_SIZE>>> blocks { };
    Slot* free_head { nullptr };
    u32 end_index { 0U };
    u32 size { 0U };

    [[nodiscard]] Slot& SlotAt(const u32 index) const { return blocks[index / BLOCK_SIZE]->operator[](index % BLOCK_SIZE); }

public:
    Hive() = default;
    Hive(const Hive&) = delete;
    Hive& operator=(const Hive&) = delete;
    Hive(Hive&&) noexcept = default;
    Hive& operator=(Hive&&) noexcept = default;
    ~Hive() { Clear(); }

    template <typename... Args> T& Emplace(Args&&... args) {
        Slot* slot = free_head;
        if (slot) {
            free_head = slot->next_free;
        } else {
            if (end_index == blocks.size() * BLOCK_SIZE) { blocks.EmplaceBack(new Array<Slot, BLOCK_SIZE>()); }
            slot = &SlotAt(end_index++);
        }
        slot->occupied = true;
        size++;
        return *std::construct_at(reinterpret_cast<T*>(slot->bytes), std::forward<Args>(args)...);
    }
    void Erase(T& item) {
        Slot& slot = *reinterpret_cast<Slot*>(std::addressof(item));
        std::destroy_at(std::addressof(item));
        slot.occupied = false;
        slot.next_free = free_head;
        free_head = &slot;
        size--;
    }
    void Clear() {
        for (T& item : *this) { std::destroy_at(std::addressof(item)); }
        blocks.clear();
        free_head = nullptr;
        end_index = 0U;
        size = 0U;
    }
    [[nodiscard]] u32 Size() const noexcept { return size; }

    template <class HiveType> struct HiveIteratorT {
        using Ref = std::conditional_t<std::is_const_v<HiveType>, const T&, T&>;
        HiveType& hive;
        u32 index;

        void SkipErased() {
            while (index < hive.end_index && !hive.SlotAt(index).occupied) { ++index; }
        }
        Ref operator*() const { return hive.SlotAt(index).Value(); }
        HiveIteratorT& operator++() {
            ++index;
            SkipErased();
            return *this;
        }
        bool operator==(const HiveIteratorT& other) const { return index == other.index; }
        bool operator!=(const HiveIteratorT& other) const { return index != other.index; }
    };
    using Iterator = HiveIteratorT<Hive>;
    using ConstIterator = HiveIteratorT<const Hive>;

    Iterator begin() {
        Iterator it { *this, 0U };
        it.SkipErased();
        return it;
    }
    Iterator end() { return { *this, end_index }; }
    ConstIterator begin() const {
        ConstIterator it { *this, 0U };
        it.SkipErased();
        return it;
    }
    ConstIterator end() const { return { *this, end_index }; }
};
template <typename K, typename V> class HiveMap {
    List<K> keys { };
    List<V*> value_pointers { };
    Hive<V> values { };

public:
    HiveMap() = default;
    template <class... Args> V& Emplace(const K& key, Args&&... args) {
        keys.EmplaceBack(key);
        return *value_pointers.EmplaceBack(&values.Emplace(std::forward<Args>(args)...));
    }
    void Erase(const K& key) {
        const u32 index = keys.IndexOf(key);
        values.Erase(*value_pointers[index]);
        keys.erase_at(index);
        value_pointers.erase_at(index);
    }
    void Clear() {
        keys.clear();
        value_pointers.clear();
        values.Clear();
    }
    [[nodiscard]] b8 HasKey(const K& key) const { return keys.Contains(key); }
    [[nodiscard]] V& operator[](const K& key) {
        const u32 pos = keys.IndexOf(key);
        assert(pos < value_pointers.size()); // HiveMap key not found
        return *value_pointers[pos];
    }
    [[nodiscard]] const V& operator[](const K& key) const {
        const u32 pos = keys.IndexOf(key);
        assert(pos < value_pointers.size()); // HiveMap key not found
        return *value_pointers[pos];
    }
    [[nodiscard]] const List<K>& Keys() const { return keys; }
    [[nodiscard]] const Hive<V>& Values() const { return values; }
};
} // namespace hex
