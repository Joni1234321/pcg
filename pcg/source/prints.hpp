#pragma once

#include "logger.hpp"

#include "components.hpp"
#include "types.hpp"
#include <string>
#include <vector>

namespace pcg {
void PrintFactory(u32 i) {
	pce::Logger logger;
	if (i >= factoryArchetype.n)
		return;
	logger.Log("Factory: {}, GOODS: {} MONEY: {:.0f}", i, factoryArchetype.goods[i], factoryArchetype.moneys[i]);
	logger.Log("Transports");
	for (u32 t : factoryArchetype.transporters[i])
		logger.Log(std::string(t, '#') + std::string(TRAVEL_COST - t, 'O'));
}
//void PrintStates(pce::Logger &logger) {
//	// i would like a syntax for this (factory -> market) -> player
//	pce::Array<pce::List<Entity>> factories_in_market_list = pce::split(factoryArchetype.markets, markets::n);
//	pce::Array<u32> player_total_factory(player::n);
//	pce::Array<pce::List<Entity>> player_market_factory(player::n);
//
//	for (Entity market = 0; market.index < markets::n; market.index++) {
//		const pce::List<Entity> &factories_in_market = factories_in_market_list[market.index];
//
//		pce::util::clear_inner(player_market_factory);
//		pce::split(player_market_factory, factories_in_market, factoryArchetype.owners);
//		for (u32 player = 0; player < player::n; player++)
//			player_total_factory[player] += player_market_factory[player].size();
//
//		pce::Array<u32> factoryCount = pce::util::get_inner_sizes(player_market_factory);
//
//		logger.LogLine();
//		logger.Log("State {:3}", market.index);
//		logger.LogList("Player", "Factory", pce::Span<u32>(factoryCount));
//	}
//	logger.LogLine();
//	logger.Log("Total Factories");
//	logger.LogList("Player", "Factory", pce::Span<u32>(player_total_factory));
//}
}