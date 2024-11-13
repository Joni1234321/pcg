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
    (void)population_balance.EmplaceBack(Money { 0.0F });
    (void)population_quality_of_life.EmplaceBack(QualityOfLife { 0.0F });
    (void)construction_queue.EmplaceBack();

    return entity;
}
b8 PlanetArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    players.SwapBack(entity);
    moneys.SwapBack(entity);
    populations.SwapBack(entity);
    population_balance.SwapBack(entity);
    construction_queue.SwapBack(entity);
    ages.SwapBack(entity);

    return true;
}
Entity PlanetArchetype::AddTemplate(const Tick tick, const PlanetTemplate planet_template) {
    Entity planet;
    switch (planet_template) {
        case PlanetTemplate::Agriculture: {
            planet = Add(tick, Money { 0.0F }, Population { 4'282'000.0F });
            const Entity wheat_farm = farm_archetype.Add(planet, FarmType::Wheat);
            farm_archetype.finances[wheat_farm].level = 1000U;
            const Entity cow_farm = farm_archetype.Add(planet, FarmType::Cows);
            farm_archetype.finances[cow_farm].level = 300U;
            const Entity wine_farm = farm_archetype.Add(planet, FarmType::Wine);
            farm_archetype.finances[wine_farm].level = 90U;
            break;
        }
        case PlanetTemplate::Playground: {
            planet = Add(tick, Money { 0.0F }, Population { 1'000.0F });
            const Entity wheat_farm = farm_archetype.Add(planet, FarmType::Wheat);
            const Entity cow_farm = farm_archetype.Add(planet, FarmType::Cows);
            const Entity wine_farm = farm_archetype.Add(planet, FarmType::Wine);
            break;
        }
        case PlanetTemplate::Gaia: {
            planet = Add(tick, Money { 10'000.0F }, Population { 100'000'000.0F });
            break;
        }
        default: // TOMB
        {
            planet = Add(tick, Money { 0.0F }, Population { 0.0F });
            break;
        }
    }
    (void)farm_archetype.Add(planet, FarmType::Construction);
    planet_archetype.construction_queue[planet].construction_capacity++;
    return planet;
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
Entity FarmSectorArchetype::Add(Entity planet, FarmType farm_type) {
    const Entity entity = Archetype::Add();

    (void)planets.EmplaceBack(planet);
    (void)type.EmplaceBack(farm_type);
    (void)finances.EmplaceBack(Finance { .level = 1U, .assets = Money { 0.0F }, .liabilities = Money { 0.0F }, .equity = Money { 0.0F } });

    return entity;
}
b8 FarmSectorArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    planets.SwapBack(entity);
    type.SwapBack(entity);

    return true;
}
} // namespace pcg
