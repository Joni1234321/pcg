#include "components.hpp"
#include "types.hpp"


//TODO: PLYER  PLANET CONCEPT CONSTRAINTS
namespace pcg {
using pce::Rand;

OptionalEntity<> PlayerArchetype::Add(const Money money) {
    const OptionalEntity<> entity = Archetype::Add();

    (void)moneys.EmplaceBack(money);
    (void)construction_queue.EmplaceBack();

    return entity;
}
b8 PlayerArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    moneys.SwapBack(entity);
    construction_queue.SwapBack(entity);

    return true;
}
OptionalEntity<> PlanetArchetype::Add(const Tick tick, const Money money) {
    const OptionalEntity<> entity = Archetype::Add();

    (void)players.EmplaceBack(Entity::NONE);
    (void)moneys.EmplaceBack(money);
    (void)construction_queue.EmplaceBack();
    (void)ages.EmplaceBack(tick);

    return entity;
}
b8 PlanetArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    moneys.SwapBack(entity);
    construction_queue.SwapBack(entity);

    return true;
}
OptionalEntity<> StateArchetype::Add(Entity player) {
    const OptionalEntity<> entity = Archetype::Add();

    constexpr u32 population = 100U;
    (void)players.EmplaceBack(player);
    markets.PushBack(Market { .population = Rand() % population });

    return entity;
}
b8 StateArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    players.SwapBack(entity);
    markets.SwapBack(entity);

    return true;
}
OptionalEntity<> FarmSectorArchetype::Add(Entity planet, FARM_TYPE farm_type) {
    const OptionalEntity<> entity = Archetype::Add();

    (void)planets.EmplaceBack(planet);
    (void)type.EmplaceBack(farm_type);

    return entity;
}
b8 FarmSectorArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    planets.SwapBack(entity);
    type.SwapBack(entity);

    return true;
}
} // namespace pcg
