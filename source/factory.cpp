#include "components.hpp"
#include "types.hpp"

namespace pcg {
using pce::Rand;

bool PlayerArchetype::Add(f32 money) {
    if (!Archetype::Add()) { return false; }

    (void)moneys.EmplaceBack(money);
    (void)construction.EmplaceBack();

    return true;
}
bool PlayerArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    moneys.swap_back(entity);

    return true;
}
bool StateArchetype::Add(Entity player) {
    if (!Archetype::Add()) { return false; }

    constexpr u32 population = 1000U;
    (void)players.EmplaceBack(player);
    markets.PushBack(Market { .population = Rand() % population });

    return true;
}
bool StateArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    players.swap_back(entity);
    markets.swap_back(entity);

    return true;
}
bool FarmSectorArchetype::Add(Entity player, FARM_TYPE farm_type) {
    if (!Archetype::Add()) { return false; }

    (void)players.EmplaceBack(player);
    (void)type.EmplaceBack(farm_type);

    return true;
}
bool FarmSectorArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    players.swap_back(entity);
    type.swap_back(entity);

    return true;
}
} // namespace pcg
