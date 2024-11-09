#include "components.hpp"
#include "types.hpp"


//TODO: PLYER  PLANET CONCEPT CONSTRAINTS
namespace pcg {
using pce::Rand;

b8 PlayerArchetype::Add(const Money money) {
    if (!Archetype::Add()) { return false; }

    (void)moneys.EmplaceBack(money);
    (void)construction_queue.EmplaceBack();

    return true;
}
b8 PlayerArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    moneys.SwapBack(entity);
    construction_queue.SwapBack(entity);

    return true;
}
b8 PlanetArchetype::Add(const Money money, const Tick tick) {
    if (!Archetype::Add()) { return false; }

    (void)players.EmplaceBack(Entity::NONE);
    (void)moneys.EmplaceBack(money);
    (void)construction_queue.EmplaceBack();
    (void)ages.EmplaceBack(tick);

    return true;
}
b8 PlanetArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    moneys.SwapBack(entity);
    construction_queue.SwapBack(entity);

    return true;
}
b8 StateArchetype::Add(Entity player) {
    if (!Archetype::Add()) { return false; }

    constexpr u32 population = 100U;
    (void)players.EmplaceBack(player);
    markets.PushBack(Market { .population = Rand() % population });

    return true;
}
b8 StateArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    players.SwapBack(entity);
    markets.SwapBack(entity);

    return true;
}
b8 FarmSectorArchetype::Add(Entity planet, FARM_TYPE farm_type) {
    if (!Archetype::Add()) { return false; }

    (void)planets.EmplaceBack(planet);
    (void)type.EmplaceBack(farm_type);

    return true;
}
b8 FarmSectorArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    planets.SwapBack(entity);
    type.SwapBack(entity);

    return true;
}
} // namespace pcg
