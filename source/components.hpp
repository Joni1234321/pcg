#pragma once

#include "collections.hpp"
#include "ecs.hpp"
#include "types.hpp"

namespace pcg {
enum class FARM_TYPE { CONSTRUCTION, WINE, WHEAT, FISH, COWS };
enum class RESOURCE_BUILDINGS { WOOD, FE, AG, AU };
enum class GOOD { WOOD, IRON, FOOD, STEEL, COAL };

struct Money : C1<f32, Money> {
    using C1::C1;
};
struct Production : C1<u32, Production> {
    using C1::C1;
};
struct BuildingUnderConstruction {
    FARM_TYPE type;
    u16 progress;
    u16 required;
    BuildingUnderConstruction(FARM_TYPE type, u16 required) : type(type), required(required), progress(0) { }
};
struct ConstructionQueue : pce::Queue<BuildingUnderConstruction> { };
struct Market {
    u32 population{};
    u32 demand{};
    u32 sold = 0;
    i32 price_factor = 0;
    [[nodiscard]] f32 GetPrice() const { return 100.0f * static_cast<f32>(price_factor); }
    void RecalculateMarkets() {
        price_factor += sold < demand ? 1 : -1;
        sold = 0;
    }
};

struct State final : Archetype {
    pce::Parent players;
    pce::Component<Market> markets;
    bool Add(Entity);
    bool Remove(Entity) override;
};
struct Sector : Archetype {
    pce::Parent players;
};
struct Farm final : Sector {
    pce::Component<FARM_TYPE> type;
    bool Add(Entity player, FARM_TYPE farm_type);
    bool Remove(Entity entity) override;
};

struct Player final : Archetype {
    pce::Component<Money> moneys;
    pce::Component<ConstructionQueue> construction;

    bool Add(f32);
    bool Remove(Entity) override;
};

inline Player player_archetype;
inline State state_archetype;
inline Farm farm_archetype;
};

#include "format"
// Logging
template <> struct std::formatter<pcg::Money> : std::formatter<f32> {
    auto format(const pcg::Money& data, std::format_context& ctx) const { return formatter<f32>::format(data.Value, ctx); }
};
template <typename EnumType> requires std::is_enum_v<EnumType>
struct std::formatter<EnumType> : std::formatter<std::underlying_type_t<EnumType>> { // NOLINT(*-dcl58-cpp)
    auto format(const EnumType& enumValue, format_context& ctx) const { return std::formatter<std::underlying_type_t<EnumType>>::format(static_cast<std::underlying_type_t<EnumType>>(enumValue), ctx); }
};
