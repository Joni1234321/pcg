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
void PlanetArchetype::AddTemplate(const Tick tick, const PlanetTemplate planet_template) {
    if (planet_template == PlanetTemplate::Agriculture) {
        Entity planet = Add(tick, Money { 0.0F }, Population { 100'000 });
        Entity wheat_farm = farm_archetype.Add(planet, FarmType::Wheat);
        farm_archetype.finances[wheat_farm].level = 1000;
        Entity cow_farm = farm_archetype.Add(planet, FarmType::Cows);
        farm_archetype.finances[cow_farm].level = 300;
        Entity wine_farm = farm_archetype.Add(planet, FarmType::Wine);
        farm_archetype.finances[wine_farm].level = 90;
    }
    else if (planet_template == PlanetTemplate::Gaia) {
        (void)Add(tick, Money { 10'000.0F }, Population { 100'000'000.0F });
    }
    else { // TOMB
        (void)Add(tick, Money { 0.0F }, Population { 0.0F });
    }
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
    (void)finances.EmplaceBack(Finance { .level = 1U, .assets = Money { 0.0F }, .liabilities = Money { 0.0F } });

    return entity;
}
b8 FarmSectorArchetype::Remove(const Entity entity) {
    if (!Archetype::Remove(entity)) { return false; }

    planets.SwapBack(entity);
    type.SwapBack(entity);

    return true;
}
} // namespace pcg
