#include "components.hpp"
#include "types.hpp"


//TODO: PLYER  PLANET CONCEPT CONSTRAINTS
namespace pcg {
using pce::Rand;

Entity PlayerArchetype::Add(const Money money) {
    const Entity entity = Archetype::Add();

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
Entity PlanetArchetype::Add(const Tick tick, const Money money, const Population population) {
    const Entity entity = Archetype::Add();

    (void)players.EmplaceBack(Entity::NONE);
    (void)ages.EmplaceBack(tick);
    (void)moneys.EmplaceBack(money);
    (void)populations.EmplaceBack(population);
    (void)construction_queue.EmplaceBack();

    return entity;
}
b8 PlanetArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    players.SwapBack(entity);
    moneys.SwapBack(entity);
    construction_queue.SwapBack(entity);
    ages.SwapBack(entity);

    return true;
}
Entity StateArchetype::Add(Entity planet) {
    const Entity entity = Archetype::Add();

    constexpr u32 population = 100U;
    (void)planets.EmplaceBack(planet);
    markets.PushBack(Market { .population = Rand() % population });

    return entity;
}
b8 StateArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    planets.SwapBack(entity);
    markets.SwapBack(entity);

    return true;
}
Entity FarmSectorArchetype::Add(Entity planet, FARM_TYPE farm_type) {
    const Entity entity = Archetype::Add();

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
