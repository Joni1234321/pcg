#pragma once

#include "components.hpp"
#include "types.hpp"
#include "util.hpp"
#include <cstdlib>
#include <vector>

namespace pcg {
namespace markets {
void AddState(const u32 pop, const u32 supply) {
	n++;
	markets.push_back(Market{ rand() % pop, rand() % supply });
}
f32 GetPrice(const Market &market) {
	return 100.0f * market.PriceFactor;
}
void RecalculateMarkets(Market &market) {
	market.PriceFactor += -1 + (market.Sold < market.Demand) * 2;
	market.Sold = 0;
}
void RecalculateMarkets() {
	for (u32 i = 0; i < n; i++)
		RecalculateMarkets(markets[i]);
}
}

bool Factory::Add(const Entity state, const u32 money, const u32 transports) {
	n++;
	moneys.emplace_back(rand() % money);

	goods.emplace_back(0);
	transporters.emplace_back(rand() % transports, 0);
	markets.emplace_back(state);

	return true;
}
bool Factory::Remove(const Entity entity) {
	n--;
	
	pce::util::SwapPop(moneys, entity.index);
	pce::util::SwapPop(transporters, entity.index);
	pce::util::SwapPop(goods, entity.index);

	return true;
}
}
