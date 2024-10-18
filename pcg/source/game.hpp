#pragma once

#include "factory.hpp"
#include "logger.hpp"
#include "player.hpp"
#include "systems.hpp"
#include "transportation.hpp"

#include "components.hpp"
#include "types.hpp"
#include <cstdlib>


namespace pcg {
namespace game {
    
    void Setup(u32 players = 2, u32 markets = 10, u32 factories = 100)
    {
        pce::Logger logger;
        //logger.Log("Starting PCG Game");
        
        for (u32 i = 0; i < players; i++) 
            player::AddPlayer();

        for (u32 i = 0; i < markets; i++)
            markets::AddState(100, 100);

        for (u32 i = 0; i < factories; i++)
            factoryArchetype.Add(rand() % markets, 1000, 30);
    }
    void Tick(u32 i)
    {
        // phases buy phase, sell phase
        factory::BuyGoods();
        transportation::MoveGoods();
        factory::BankruptFactories();
        markets::RecalculateMarkets();
        player::DoAction();
        //player::Tax(100);
    }
}
}