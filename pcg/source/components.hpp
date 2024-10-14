#pragma once

#include "collections.hpp"
#include "types.hpp"


namespace pcg {
namespace markets {
    u32 n;
    struct Market {
        u32 Population;
        u32 Demand;
        u32 Sold = 0;
        i32 PriceFactor = 0;
    };
    pce::Component<Market> markets;
}
namespace factory {
    u32 n;
    pce::Parents owners;
    pce::Parents markets;

    pce::Component<f32> moneys;
    pce::Component<u32> goods;
    pce::Component<pce::List<u32>> transporters;
}
namespace player {
    u32 n;
    pce::Component<f32> moneys;
}
const u32 TRAVEL_COST = 10;
};