#pragma once

#include "components.hpp"
#include "logger.hpp"
#include "util.hpp"
#include <vector>

namespace pcg {
namespace markets {
    void AddState(const u32 pop, const u32 supply)
    {
        n++;
        markets.push_back(Market{ rand() % pop, rand() % supply });
    }
    f32 GetPrice(const Market &market) {
        return 100 * market.PriceFactor;
    }
    void RecalculateMarkets(Market& market) {
        market.PriceFactor += -1 + (market.Sold < market.Demand) * 2;
        market.Sold = 0;
    }
    void RecalculateMarkets() {
        for (i32 i = 0; i < n; i++)
            RecalculateMarkets(markets[i]);
    }
}
namespace factory {
    void AddFactory(const Entity player, const Entity state, const i32 money, const i32 transports)
    {
        n++;
        moneys.emplace_back(rand() % money);

        goods.emplace_back(0);
        transporters.emplace_back(rand() % transports, 0);
        markets.emplace_back(state);
        owners.emplace_back(player);
    }
    void RemoveFactories(const std::vector<Entity>& i)
    {
        pce::util::SwapPop(moneys, i);
        pce::util::SwapPop(transporters, i);
        pce::util::SwapPop(goods, i);

        n -= i.size();
    }
    void BuyGoods(f32& money, u32& good)
    {
        // money -= rand() % 10;
        good += 1;
    }
    void BuyGoods()
    {
        for (u32 i = 0; i < n; i++)
            BuyGoods(moneys[i], goods[i]);
    }
    u32 BankruptFactories()
    {
        static std::vector<Entity> entitiesBuffer;
        entitiesBuffer.clear();

        for (u32 i = 0; i < n; i++)
            if (moneys[i] <= 0)
                entitiesBuffer.emplace_back(i);

        const u32 bankruptCount = entitiesBuffer.size();
        RemoveFactories(entitiesBuffer);

        pce::Logger logger;
        //if (bankruptCount) logger.Log("%d bankrupt factories", bankruptCount);
        return bankruptCount;
    }
}
}
