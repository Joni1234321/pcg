export module rail.defines;

import std;

import pce.std;

import rail.types;

export namespace rail {
constexpr GoodDefineId GOOD_GRAIN { 0 };
constexpr GoodDefineId GOOD_ALCOHOL { 1 };
constexpr GoodDefineId GOOD_WOOL { 2 };
constexpr GoodDefineId GOOD_CLOTHES { 3 };
constexpr GoodDefineId GOOD_WOOD { 4 };
constexpr GoodDefineId GOOD_LUMBER { 5 };
constexpr GoodDefineId GOOD_FURNITURE { 6 };

constexpr IndustryDefineId INDUSTRY_FARM { 0 };
constexpr IndustryDefineId INDUSTRY_BREWERY { 1 };
constexpr IndustryDefineId INDUSTRY_SHEEP_FARM { 2 };
constexpr IndustryDefineId INDUSTRY_TEXTILE_MILL { 3 };
constexpr IndustryDefineId INDUSTRY_FOREST { 4 };
constexpr IndustryDefineId INDUSTRY_SAWMILL { 5 };
constexpr IndustryDefineId INDUSTRY_FURNITURE_FACTORY { 6 };

constexpr BuildingDefineId BUILDING_HOUSE { 0 };
constexpr BuildingDefineId BUILDING_TAVERN { 1 };
constexpr BuildingDefineId BUILDING_OFFICE { 2 };
constexpr BuildingDefineId BUILDING_SCHOOL { 3 };
constexpr BuildingDefineId BUILDING_CHURCH { 4 };
constexpr BuildingDefineId BUILDING_HOSPITAL { 5 };

std::vector<GoodDefine> RailGoodDefines() {
    return {
        GoodDefine { .id = GOOD_GRAIN, .name = "Grain", .value = 10.0F, .attrition = 0.2F },
        GoodDefine { .id = GOOD_ALCOHOL, .name = "Alcohol", .value = 40.0F, .attrition = 0.05F },
        GoodDefine { .id = GOOD_WOOL, .name = "Wool", .value = 15.0F, .attrition = 0.1F },
        GoodDefine { .id = GOOD_CLOTHES, .name = "Clothes", .value = 60.0F, .attrition = 0.05F },
        GoodDefine { .id = GOOD_WOOD, .name = "Wood", .value = 8.0F, .attrition = 0.05F },
        GoodDefine { .id = GOOD_LUMBER, .name = "Lumber", .value = 20.0F, .attrition = 0.02F },
        GoodDefine { .id = GOOD_FURNITURE, .name = "Furniture", .value = 80.0F, .attrition = 0.02F },
    };
}

std::vector<IndustryDefine> RailIndustryDefines() {
    return {
        IndustryDefine { .id = INDUSTRY_FARM, .name = "Farm", .cost_initial = 500.0F, .cost_labour = 50.0F, .demand = {}, .supply = { GOOD_GRAIN }, .production_rate = 1.0F },
        IndustryDefine { .id = INDUSTRY_BREWERY, .name = "Brewery", .cost_initial = 2000.0F, .cost_labour = 150.0F, .demand = { GOOD_GRAIN }, .supply = { GOOD_ALCOHOL }, .production_rate = 1.0F },
        IndustryDefine { .id = INDUSTRY_SHEEP_FARM, .name = "Sheep Farm", .cost_initial = 500.0F, .cost_labour = 50.0F, .demand = {}, .supply = { GOOD_WOOL }, .production_rate = 1.0F },
        IndustryDefine { .id = INDUSTRY_TEXTILE_MILL, .name = "Textile Mill", .cost_initial = 3000.0F, .cost_labour = 200.0F, .demand = { GOOD_WOOL }, .supply = { GOOD_CLOTHES }, .production_rate = 1.0F },
        IndustryDefine { .id = INDUSTRY_FOREST, .name = "Forest", .cost_initial = 300.0F, .cost_labour = 40.0F, .demand = {}, .supply = { GOOD_WOOD }, .production_rate = 1.0F },
        IndustryDefine { .id = INDUSTRY_SAWMILL, .name = "Sawmill", .cost_initial = 1500.0F, .cost_labour = 100.0F, .demand = { GOOD_WOOD }, .supply = { GOOD_LUMBER }, .production_rate = 1.0F },
        IndustryDefine { .id = INDUSTRY_FURNITURE_FACTORY, .name = "Furniture Factory", .cost_initial = 3000.0F, .cost_labour = 200.0F, .demand = { GOOD_LUMBER }, .supply = { GOOD_FURNITURE }, .production_rate = 1.0F },
    };
}

std::vector<BuildingDefineId> RailCityGrowthSequence() {
    return { BUILDING_HOUSE, BUILDING_HOUSE, BUILDING_HOUSE, BUILDING_TAVERN, BUILDING_HOUSE, BUILDING_HOUSE, BUILDING_SCHOOL, BUILDING_HOUSE, BUILDING_HOUSE, BUILDING_OFFICE, BUILDING_HOUSE, BUILDING_CHURCH, BUILDING_HOUSE, BUILDING_HOUSE, BUILDING_OFFICE, BUILDING_HOSPITAL };
}

std::vector<BuildingDefine> RailBuildingDefines() {
    return {
        BuildingDefine { .id = BUILDING_HOUSE, .name = "House", .demand = { { GOOD_CLOTHES, 0.5F }, { GOOD_FURNITURE, 0.2F } }, .supply = {}, .size_hex_widths = 0.25F },
        BuildingDefine { .id = BUILDING_TAVERN, .name = "Tavern", .demand = { { GOOD_ALCOHOL, 1.0F } }, .supply = {}, .size_hex_widths = 0.3F },
        BuildingDefine { .id = BUILDING_OFFICE, .name = "Office", .demand = { { GOOD_FURNITURE, 0.6F } }, .supply = {}, .size_hex_widths = 0.45F },
        BuildingDefine { .id = BUILDING_SCHOOL, .name = "School", .demand = { { GOOD_FURNITURE, 0.4F } }, .supply = {}, .size_hex_widths = 0.55F },
        BuildingDefine { .id = BUILDING_CHURCH, .name = "Church", .demand = { { GOOD_LUMBER, 0.2F } }, .supply = {}, .size_hex_widths = 0.35F },
        BuildingDefine { .id = BUILDING_HOSPITAL, .name = "Hospital", .demand = { { GOOD_CLOTHES, 0.8F } }, .supply = {}, .size_hex_widths = 0.6F },
    };
}
} // namespace rail
