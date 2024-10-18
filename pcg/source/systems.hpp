#pragma once

#include "components.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <vector>

namespace pcg {
namespace factory {

void BuyGoods(Money &money, Good &good) {
	// money -= rand() % 10;
	good = good + 1;
}
void BuyGoods() {
	for (u32 i = 0; i < factoryArchetype.n; i++)
		BuyGoods(factoryArchetype.moneys[i], factoryArchetype.goods[i]);
}
u32 BankruptFactories() {
	static std::vector<Entity> entitiesBuffer;
	entitiesBuffer.clear();

	for (u32 i = 0; i < factoryArchetype.n; i++)
		if (factoryArchetype.moneys[i].Value <= 0)
			entitiesBuffer.emplace_back(i);

	std::reverse(entitiesBuffer.begin(), entitiesBuffer.end());
	const u32 bankruptCount = entitiesBuffer.size();
	for (const Entity e : entitiesBuffer)
		factoryArchetype.Remove(e);

	pce::Logger logger;
	//if (bankruptCount) logger.Log("%d bankrupt factories", bankruptCount);
	return bankruptCount;
}
}
}