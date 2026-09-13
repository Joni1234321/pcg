module;

#include "SDL3_image/SDL_image.h"

export module rail.scenarios;

import std;

import pce.std;
import pce.math;
import pce.assets;
import pce.logger;

import hex.hex;
import rail.types;

using namespace hex;

export namespace rail {
constexpr f32 CITY_RADIUS_WORLD_PER_LEVEL = 1.5F;
constexpr u32 BUILDINGS_PER_CITY_LEVEL = 6U;
constexpr u32 INDUSTRIES_PER_CITY_LEVEL = 1U;
constexpr u32 PLACEMENT_ATTEMPTS = 16U;
constexpr f32 BUILDING_GAP_WORLD = 0.08F;

[[nodiscard]] b8 SquaresOverlap(const float2 world_a, const f32 half_a, const f32 rotation_a, const float2 world_b, const f32 half_b, const f32 rotation_b) {
    for (const f32 axis_rotation : { rotation_a, rotation_a + math::PI * 0.5F, rotation_b, rotation_b + math::PI * 0.5F }) {
        const float2 axis { math::Cos(axis_rotation), math::Sin(axis_rotation) };
        const f32 extent_a = half_a * (math::Abs(math::Cos(axis_rotation - rotation_a)) + math::Abs(math::Sin(axis_rotation - rotation_a)));
        const f32 extent_b = half_b * (math::Abs(math::Cos(axis_rotation - rotation_b)) + math::Abs(math::Sin(axis_rotation - rotation_b)));
        if (math::Abs(math::Dot(world_b - world_a, axis)) > extent_a + extent_b) { return false; }
    }
    return true;
}

void TerrainSave(const HexList<i8>& elevation, const AssetPath& asset_path) {
    std::ofstream file { Asset(asset_path) };
    file << elevation.map_size.x << ' ' << elevation.map_size.y << '\n';
    for (u32 y = 0; y < elevation.map_size.y; y++) {
        for (u32 x = 0; x < elevation.map_size.x; x++) { file << static_cast<i32>(elevation.data[y * elevation.map_size.x + x]) << (x + 1 < elevation.map_size.x ? ' ' : '\n'); }
    }
}

HexList<i8> TerrainLoad(const AssetPath& asset_path) {
    std::ifstream file { Asset(asset_path) };
    uint2 map_size { 0, 0 };
    file >> map_size.x >> map_size.y;
    HexList<i8> elevation;
    elevation.Resize(map_size);
    for (i8& elevation_value : elevation) {
        i32 parsed = 0;
        file >> parsed;
        elevation_value = static_cast<i8>(parsed);
    }
    return elevation;
}

void CitiesSave(const std::vector<City>& cities, const AssetPath& asset_path) {
    std::ofstream file { Asset(asset_path) };
    for (const City& city : cities) { file << "city " << city.axial.x << ' ' << city.axial.y << ' ' << city.level << ' ' << city.name << '\n'; }
}

std::vector<City> CitiesLoad(const AssetPath& asset_path) {
    std::ifstream file { Asset(asset_path) };
    std::vector<City> cities;
    std::string keyword;
    while (file >> keyword) {
        if (keyword != "city") { continue; }
        City city;
        file >> city.axial.x >> city.axial.y >> city.level >> std::ws;
        std::getline(file, city.name);
        cities.push_back(std::move(city));
    }
    return cities;
}

void RiversSave(const std::vector<River>& rivers, const AssetPath& asset_path) {
    std::ofstream file { Asset(asset_path) };
    for (const River& river : rivers) {
        file << "river " << static_cast<u32>(river.size) << ' ' << river.name << '\n';
        for (const int2 axial : river.axials) { file << axial.x << ' ' << axial.y << '\n'; }
    }
}

std::vector<River> RiversLoad(const AssetPath& asset_path) {
    std::ifstream file { Asset(asset_path) };
    std::vector<River> rivers;
    std::string token;
    while (file >> token) {
        if (token == "river") {
            u32 size = 0;
            file >> size >> std::ws;
            River river { .size = static_cast<u8>(size) };
            std::getline(file, river.name);
            rivers.push_back(std::move(river));
        } else if (!rivers.empty()) {
            int2 axial { std::stoi(token), 0 };
            file >> axial.y;
            rivers.back().axials.push_back(axial);
        }
    }
    return rivers;
}

void WaterLabelsSave(const std::vector<MapLabel>& labels, const AssetPath& asset_path) {
    std::ofstream file { Asset(asset_path) };
    for (const MapLabel& label : labels) { file << "label " << label.axial.x << ' ' << label.axial.y << ' ' << label.name << '\n'; }
}

std::vector<MapLabel> WaterLabelsLoad(const AssetPath& asset_path) {
    std::ifstream file { Asset(asset_path) };
    std::vector<MapLabel> labels;
    std::string keyword;
    while (file >> keyword) {
        if (keyword != "label") { continue; }
        MapLabel label;
        file >> label.axial.x >> label.axial.y >> std::ws;
        std::getline(file, label.name);
        labels.push_back(std::move(label));
    }
    return labels;
}

HexList<i8> ElevationFromImage(const AssetPath& asset_path, const uint2 map_size) {
    HexList<i8> elevation;
    elevation.Resize(map_size);
    SDL_Surface* loaded = IMG_Load(Asset(asset_path).string().c_str());
    if (!loaded) {
        Logger().Error("Image FAILED to load: {}", asset_path.string());
        return elevation;
    }
    SDL_Surface* surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    const float2 world_size { map_size.x * HEX_SPACING.x, (map_size.y - 1) * HEX_SPACING.y };
    for (u32 i = 0; i < elevation.Size(); i++) {
        const float2 world = HexAxialToWorld(elevation.IndexToAxial(i));
        const i32 pixel_x = static_cast<i32>(math::Clamp(world.x / world_size.x, 0.0F, 1.0F) * (surface->w - 1));
        const i32 pixel_y = static_cast<i32>(math::Clamp(world.y / world_size.y, 0.0F, 1.0F) * (surface->h - 1));
        const u8* pixel = static_cast<const u8*>(surface->pixels) + pixel_y * surface->pitch + pixel_x * 4;
        const i32 r = pixel[0];
        const i32 g = pixel[1];
        const i32 b = pixel[2];
        const b8 is_sea = b >= r + 30 && b >= g;
        const i32 luminance = (r * 299 + g * 587 + b * 114) / 1000;
        elevation.data[i] = static_cast<i8>(is_sea ? std::clamp(-1 - (220 - luminance) * 127 / 100, static_cast<i32>(ELEVATION_MIN), -1) : std::clamp((r - 140) * ELEVATION_MAX / 105, 0, static_cast<i32>(ELEVATION_MAX)));
    }
    SDL_DestroySurface(surface);
    return elevation;
}

Map MapGenerate(const MapDefine& define, const std::vector<IndustryDefine>& industry_defines, const std::vector<BuildingDefine>& building_defines, const std::vector<BuildingDefineId>& city_growth) {
    Map map { .water_labels = define.water_labels, .rivers = define.rivers, .cities = define.cities, .industries = define.industries };
    map.size = HexAxialToWorld(HexOffsetToAxial(static_cast<int2>(define.elevation.map_size - uint2 { 1U, 1U })));
    const auto random_land_world = [&define](const float2 world_center, const f32 world_radius_min, const f32 world_radius_max) -> Optional<float2> {
        for (u32 attempt = 0; attempt < PLACEMENT_ATTEMPTS; attempt++) {
            const f32 angle = RandF(0.0F, math::PI * 2.0F);
            const float2 world = world_center + float2 { math::Cos(angle), math::Sin(angle) } * float2 { RandF(world_radius_min, world_radius_max) };
            const int2 axial = HexWorldToAxial(world);
            if (define.elevation.Contains(axial) && define.elevation[axial] >= 0) { return world; }
        }
        return std::nullopt;
    };
    for (const City& city : define.cities) {
        const float2 world_city = HexAxialToWorld(city.axial);
        const f32 world_radius = city.level * CITY_RADIUS_WORLD_PER_LEVEL;
        const auto building_half_world = [&building_defines](const BuildingDefineId id) { return building_defines[id.value].size_hex_widths * HEX_SPACING.x * 0.5F + BUILDING_GAP_WORLD * 0.5F; };
        const u32 city_buildings_first = static_cast<u32>(map.buildings.size());
        for (u32 i = 0; i < static_cast<u32>(city.level * BUILDINGS_PER_CITY_LEVEL); i++) {
            const BuildingDefineId id = city_growth[i % city_growth.size()];
            for (u32 attempt = 0; attempt < PLACEMENT_ATTEMPTS; attempt++) {
                const Optional<float2> world = random_land_world(world_city, 0.0F, world_radius);
                if (!world.has_value()) { continue; }
                const f32 rotation = RandF(0.0F, math::PI * 0.5F);
                const auto overlaps = [&](const Building& other) { return SquaresOverlap(*world, building_half_world(id), rotation, other.pos, building_half_world(other.id), other.rotation); };
                if (std::any_of(map.buildings.begin() + city_buildings_first, map.buildings.end(), overlaps)) { continue; }
                map.buildings.push_back(Building { .pos = *world, .rotation = rotation, .id = id });
                break;
            }
        }
        for (u32 i = 0; i < static_cast<u32>(math::Ceil(city.level * INDUSTRIES_PER_CITY_LEVEL)); i++) {
            if (const Optional<float2> world = random_land_world(world_city, world_radius, world_radius * 2.0F)) { map.industries.push_back(Industry { .pos = *world, .id = industry_defines[Rand(static_cast<u32>(industry_defines.size()))].id }); }
        }
    }
    return map;
}

std::vector<MapDefine> RailMapDefines() {
    return {
        MapDefine {
            .elevation = TerrainLoad("rail/scenarios_base/britain.txt"),
            .water_labels = WaterLabelsLoad("rail/scenarios_base/britain_water_labels.txt"),
            .rivers = RiversLoad("rail/scenarios_base/britain_rivers.txt"),
            .cities = CitiesLoad("rail/scenarios_base/britain_cities_1830.txt"),
        },
        MapDefine {
            .elevation = TerrainLoad("rail/scenarios_base/twin_cities.txt"),
            .rivers = { River { .size = 6U, .axials = { int2 { -4, 8 }, int2 { 0, 8 }, int2 { 4, 8 }, int2 { 8, 8 }, int2 { 11, 8 } }, .name = "Twin River" } },
            .cities = CitiesLoad("rail/scenarios_base/twin_cities_cities_1830.txt"),
        },
    };
}
} // namespace rail
