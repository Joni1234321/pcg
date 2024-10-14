#pragma once

#include "factory.hpp"
#include "util.hpp"
#include <vector>

namespace pcg {
namespace transportation {
    void MoveGoods(std::vector<u32>& transport, pcg::markets::Market& market, u32& good, f32& money)
    {
        const u32 n = transport.size();
        const f32 price = markets::GetPrice(market);
        for (u32 i = 0; i < n; i++) {
            if (transport[i] == 1) {
                market.Sold++;
                money += price;
            }
            const u32 speed = 1;
            transport[i] = pce::util::sub_safe(transport[i], speed);
        }

        if (good == 0)
            return;

        for (u32 i = 0; i < n; i++) {
            if (transport[i] > 0)
                continue;

            transport[i] = TRAVEL_COST;
            good -= 1;
            break; 
        }
    }
    void MoveGoods()
    {
        for (u32 i = 0; i < factory::n; i++) {
            Entity state = factory::markets[i];
            MoveGoods(factory::transporters[i], markets::markets[state.index], factory::goods[i], factory::moneys[i]);
        }
    }
}
}
