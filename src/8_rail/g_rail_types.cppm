export module rail.types;

import std;

import pce.std;
import pce.strong;

export namespace rail {

using GoodDefineId = hex::StrongType<u32, struct GoodDefineIdTag>;
struct GoodDefine {
    GoodDefineId id;
    std::string name;
    f32 value; // value per ton
    f32 attrition; // attrition per year
};
using IndustryDefineId = hex::StrongType<u32, struct IndustryDefineIdTag>;
struct IndustryDefine {
    IndustryDefineId id;
    std::string name;
    f32 cost_initial;
    f32 cost_labour;
    std::vector<GoodDefineId> demand;
    std::vector<GoodDefineId> supply;
    f32 production_rate;
};
using BuildingDefineId = hex::StrongType<u32, struct BuildingDefineIdTag>;
struct BuildingDefine {
    BuildingDefineId id;
    std::string name;
    std::flat_map<GoodDefineId, f32> demand;
    std::flat_map<GoodDefineId, f32> supply;
};



struct GoodInfo {
    f32 amount;
    f32 price;
};

struct Industry {
    float2 pos;
    IndustryDefineId id;
};
struct City {
    float2 pos;
    std::string name;
};
struct River {
    std::vector<float2> waypoints;
};
struct Building {
    float2 pos;
    BuildingDefineId id;
};
struct MapDefine {
    std::vector<i8> elevation;  // grid
    std::vector<River> rivers;
    std::vector<City> cities;
    std::vector<Industry> industries;
};

struct Map {
    std::vector<i8> elevation;  // grid
    std::vector<River> rivers;
    std::vector<City> cities;
    std::vector<Building> buildings;
    std::vector<Industry> industries;

    std::flat_map<GoodDefineId, std::vector<GoodInfo>> goods; // grid
    ///
};
}