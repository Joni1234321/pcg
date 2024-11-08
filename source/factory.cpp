#include "components.hpp"
#include "types.hpp"

namespace pcg {
using pce::Rand;

bool PlayerArchetype::Add(const Money money) {
    if (!Archetype::Add()) { return false; }

    (void)moneys.EmplaceBack(money);
    (void)construction_queue.EmplaceBack();

    return true;
}
bool PlayerArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    moneys.SwapBack(entity);
    construction_queue.SwapBack(entity);

    return true;
}
bool PlanetArchetype::Add(const Money money, const Tick tick) {
    if (!Archetype::Add()) { return false; }

    (void)moneys.EmplaceBack(money);
    (void)construction_queue.EmplaceBack();
    (void)ages.EmplaceBack(tick);

    return true;
}
bool PlanetArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    moneys.SwapBack(entity);
    construction_queue.SwapBack(entity);

    return true;
}
bool StateArchetype::Add(Entity player) {
    if (!Archetype::Add()) { return false; }

    constexpr u32 population = 100U;
    (void)players.EmplaceBack(player);
    markets.PushBack(Market { .population = Rand() % population });

    return true;
}
bool StateArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    players.SwapBack(entity);
    markets.SwapBack(entity);

    return true;
}
bool FarmSectorArchetype::Add(Entity planet, FARM_TYPE farm_type) {
    if (!Archetype::Add()) { return false; }

    (void)planets.EmplaceBack(planet);
    (void)type.EmplaceBack(farm_type);

    return true;
}
bool FarmSectorArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    planets.SwapBack(entity);
    type.SwapBack(entity);

    return true;
}
} // namespace pcg
