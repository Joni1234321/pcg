#pragma once

#include <cstdlib>
#include <vector>

#include "components.hpp"
#include "types.hpp"

namespace pcg {
bool Player::Add(f32 money) {
    if (!Archetype::Add()) return false;

    moneys.EmplaceBack(money);
    construction.EmplaceBack();

    return true;
}
bool Player::Remove(Entity entity) {
    if (!Archetype::Remove(entity)) return false;

    moneys.swap_back(entity.index);

    return true;
}

bool State::Add(Entity player) {
    if (!Archetype::Add()) return false;

    const u32 POP = 1000;
    players.EmplaceBack(player);
    markets.PushBack(Market {pce::rand() % POP});

    return true;
}
bool State::Remove(Entity entity) {
    if (!Archetype::Remove(entity)) return false;

    players.swap_back(entity.index);
    markets.swap_back(entity.index);

    return true;
}

bool Farm::Add(Entity player, FARM_TYPE farm_type) {
    if (!Archetype::Add()) return false;

    players.EmplaceBack(player);
    type.EmplaceBack(farm_type);

    return true;
}
bool Farm::Remove(Entity entity) {
    if (!Archetype::Remove(entity)) return false;

    players.swap_back(entity.index);
    type.swap_back(entity.index);

    return true;
}
} // namespace pcg
