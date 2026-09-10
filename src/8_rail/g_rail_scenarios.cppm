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
    for (const City& city : cities) { file << "city " << city.axial.x << ' ' << city.axial.y << ' ' << city.name << '\n'; }
}

std::vector<City> CitiesLoad(const AssetPath& asset_path) {
    std::ifstream file { Asset(asset_path) };
    std::vector<City> cities;
    std::string keyword;
    while (file >> keyword) {
        if (keyword != "city") { continue; }
        City city;
        file >> city.axial.x >> city.axial.y >> std::ws;
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
