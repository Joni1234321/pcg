#pragma once

#include "factory.hpp"
#include "player.hpp"
#include "timer.hpp"
#include "transportation.hpp"

#include <algorithm>
#include <numeric>


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
            factory::AddFactory(rand() % players, rand() % markets, 1000, 30);
    }
    void Tick(u32 i)
    {
        factory::BuyGoods();
        transportation::MoveGoods();
        factory::BankruptFactories();
        markets::RecalculateMarkets();
        player::DoAction();
        player::Tax(100);
    }
}
}