#pragma once

#include "collections.hpp"
#include "components.hpp"
#include "factory.hpp"
#include "types.hpp"
#include "util.hpp"
#include <vector>

namespace pcg {
namespace transportation {
void MoveGoods(pce::List<Transporter> &transport, pcg::markets::Market &market, Good &good, Money &money) {
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
void MoveGoods() {
	for (u32 i = 0; i < factoryArchetype.n; i++) {
		Entity state = factoryArchetype.markets[i];
		MoveGoods(factoryArchetype.transporters[i], markets::markets[state.index], factoryArchetype.goods[i], factoryArchetype.moneys[i]);
	}
}
}
}
