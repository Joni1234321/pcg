export module rail.types;

import std;

import pce.std;
import pce.strong;

import hex.hex;

export namespace rail {
constexpr i8 ELEVATION_MIN = -128;
constexpr i8 ELEVATION_MAX = 127;

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
    int2 axial;
    std::string name;
};
struct MapLabel {
    int2 axial;
    std::string name;
};
struct River {
    u8 size;
    std::vector<int2> axials;
    std::string name;
};
struct Building {
    float2 pos;
    BuildingDefineId id;
};
struct MapDefine {
    hex::HexList<i8> elevation;
    std::vector<MapLabel> water_labels;
    std::vector<River> rivers;
    std::vector<City> cities;
    std::vector<Industry> industries;
};

struct Map {
    float2 size;
    std::vector<i8> elevation;  // grid
    std::vector<MapLabel> water_labels;
    std::vector<River> rivers;
    std::vector<City> cities;
    std::vector<Building> buildings;
    std::vector<Industry> industries;

    std::flat_map<GoodDefineId, std::vector<GoodInfo>> goods; // grid
};
}