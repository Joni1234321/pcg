module;

#include <cassert>
#include "SDL3/SDL.h"

export module hex.render;

import std;

import pce.globals;
import pce.window_state;
import pce.assets;
import pce.sdl;
import pcs.camera;
import pce.colors;
import pce.std;
import hex.hex;
import hex.types;
import pce.collections;
import pce.math;

export namespace pcg {
inline void VertHexAppend(List<Vertex>& vertecies, const f32 hex_size, const float2 hex_screen, const ColorF hex_color, const Optional<ColorF> hex_color_inner = std::nullopt) {
    Array<float2, HEX_CORNERS> points { };
    for (u32 i = 0; i < HEX_CORNERS; i++) { points[i] = hex_screen + HEX_ANGLE[i] * float2 { hex_size }; }
    for (u32 i = 0; i < HEX_CORNERS; i++) {
        vertecies.EmplaceBack(hex_screen, hex_color_inner.value_or(colors::ColorMul(hex_color, 1.2F)));
        vertecies.EmplaceBack(points[i], hex_color);
        vertecies.EmplaceBack(points[(i + 1) % HEX_CORNERS], hex_color);
    }
}
inline void VertAabbAppend(List<Vertex>& vertices, const AABB& aabb, const ColorF color) {
    const Array<float2, 4> points { {
        aabb.point + float2 { -aabb.size.x, aabb.size.y },
        aabb.point + float2 { aabb.size.x, aabb.size.y },
        aabb.point + float2 { aabb.size.x, -aabb.size.y },
        aabb.point + float2 { -aabb.size.x, -aabb.size.y },
    } };

    vertices.EmplaceBack(Vertex { points[0], color });
    vertices.EmplaceBack(Vertex { points[1], color });
    vertices.EmplaceBack(Vertex { points[2], color });
    vertices.EmplaceBack(Vertex { points[0], color });
    vertices.EmplaceBack(Vertex { points[2], color });
    vertices.EmplaceBack(Vertex { points[3], color });
}
inline void VertObbAppend(List<Vertex>& vertices, const OBB& obb, const ColorF color) {
    const float2 half_size = obb.size * float2 { 0.5F };
    const Array<float2, 4> points { {
        obb.center + math::Rotate(float2 { -half_size.x, half_size.y }, obb.angle),
        obb.center + math::Rotate(float2 { half_size.x, half_size.y }, obb.angle),
        obb.center + math::Rotate(float2 { half_size.x, -half_size.y }, obb.angle),
        obb.center + math::Rotate(float2 { -half_size.x, -half_size.y }, obb.angle),
    } };
    vertices.EmplaceBack(Vertex { points[0], color });
    vertices.EmplaceBack(Vertex { points[1], color });
    vertices.EmplaceBack(Vertex { points[2], color });
    vertices.EmplaceBack(Vertex { points[0], color });
    vertices.EmplaceBack(Vertex { points[2], color });
    vertices.EmplaceBack(Vertex { points[3], color });
}

[[nodiscard]] float2 HexTileJitter(const int2 axial) {
    const u32 h = pce::noise::Hash(axial.x, axial.y);
    const f32 fx = (static_cast<f32>(h & 0xFFFFU) / 65535.0F) * 2.0F - 1.0F;
    const f32 fy = (static_cast<f32>((h >> 16) & 0xFFFFU) / 65535.0F) * 2.0F - 1.0F;
    return float2 { fx, fy };
}

[[nodiscard]] constexpr TerrainType FloatToTerrainType(const f32 terrain) {
    if (terrain < 0.25F) { return TerrainType::TERRAIN_TYPE_DEEP_OCEAN; }
    if (terrain < 0.38F) { return TerrainType::TERRAIN_TYPE_OCEAN; }
    if (terrain < 0.43F) { return TerrainType::TERRAIN_TYPE_BEACH; }
    if (terrain < 0.60F) { return TerrainType::TERRAIN_TYPE_GRASS; }
    if (terrain < 0.72F) { return TerrainType::TERRAIN_TYPE_HILL; }
    if (terrain < 0.85F) { return TerrainType::TERRAIN_TYPE_MOUNTAIN; }
    return TerrainType::TERRAIN_TYPE_SNOW;
}
[[nodiscard]] constexpr Color TerrainToColor(const TerrainType terrain) {
    switch (terrain) {
        case TerrainType::TERRAIN_TYPE_DEEP_OCEAN: return Color { 20U, 60U, 120U };
        case TerrainType::TERRAIN_TYPE_OCEAN: return Color { 50U, 100U, 180U };
        case TerrainType::TERRAIN_TYPE_BEACH: return colors::COLOR_KHAKI;
        case TerrainType::TERRAIN_TYPE_GRASS: return Color { 100U, 190U, 80U };
        case TerrainType::TERRAIN_TYPE_HILL: return colors::COLOR_FOREST_GREEN;
        case TerrainType::TERRAIN_TYPE_MOUNTAIN: return colors::COLOR_GRAY;
        case TerrainType::TERRAIN_TYPE_SNOW: return colors::COLOR_WHITE;
    }
    std::unreachable();
}

[[nodiscard]] constexpr Color TerrainToColorScheme(const TerrainType terrain) {
    if constexpr (TERRAIN_SCHEME == MapStyle::CIV_VIBRANT) {
        return TerrainToColor(terrain);
    } else if constexpr (TERRAIN_SCHEME == MapStyle::SLATE_TABLE) {
        switch (terrain) {
            case TerrainType::TERRAIN_TYPE_DEEP_OCEAN: return Color { 28U, 38U, 55U };
            case TerrainType::TERRAIN_TYPE_OCEAN: return Color { 52U, 72U, 96U };
            case TerrainType::TERRAIN_TYPE_BEACH: return Color { 156U, 144U, 110U };
            case TerrainType::TERRAIN_TYPE_GRASS: return Color { 102U, 122U, 86U };
            case TerrainType::TERRAIN_TYPE_HILL: return Color { 62U, 84U, 60U };
            case TerrainType::TERRAIN_TYPE_MOUNTAIN: return Color { 92U, 92U, 100U };
            case TerrainType::TERRAIN_TYPE_SNOW: return Color { 198U, 204U, 212U };
        }
    } else {
        switch (terrain) {
            case TerrainType::TERRAIN_TYPE_DEEP_OCEAN: return Color { 88U, 112U, 142U };
            case TerrainType::TERRAIN_TYPE_OCEAN: return Color { 130U, 158U, 184U };
            case TerrainType::TERRAIN_TYPE_BEACH: return Color { 220U, 200U, 158U };
            case TerrainType::TERRAIN_TYPE_GRASS: return Color { 178U, 184U, 132U };
            case TerrainType::TERRAIN_TYPE_HILL: return Color { 128U, 144U, 96U };
            case TerrainType::TERRAIN_TYPE_MOUNTAIN: return Color { 152U, 138U, 116U };
            case TerrainType::TERRAIN_TYPE_SNOW: return Color { 232U, 226U, 210U };
        }
    }
    std::unreachable();
}
[[nodiscard]] constexpr Color CountryTagToColor(const CountryTag tag) {
    switch (tag) {
        case CountryTag::TAG_GER: return colors::COLOR_WG_GER_BG;
        case CountryTag::TAG_SOV: return colors::COLOR_WG_SOV_BG;
        case CountryTag::TAG_USA: return colors::COLOR_WG_USA_BG;
        case CountryTag::TAG_NONE: return colors::COLOR_WHITE;
    }
    assert(false);
    std::unreachable();
}

struct TerrainFeatureTextures {
    HandleOptional<Texture> grassland;
    HandleOptional<Texture> field;
    HandleOptional<Texture> city;
    HandleOptional<Texture> village;
    HandleOptional<Texture> wooded_lightly;
    HandleOptional<Texture> wooded_heavy;
    HandleOptional<Texture> marsh;

    explicit TerrainFeatureTextures(const RelativePath& dir) {
        grassland = pce::globalData.Create<pce::Texture>(pce::Asset(dir / "grassland.png"));
        field = pce::globalData.Create<pce::Texture>(pce::Asset(dir / "field.png"));
        city = pce::globalData.Create<pce::Texture>(pce::Asset(dir / "city.png"));
        village = pce::globalData.Create<pce::Texture>(pce::Asset(dir / "village.png"));
        wooded_lightly = pce::globalData.Create<pce::Texture>(pce::Asset(dir / "wooded-lightly.png"));
        wooded_heavy = pce::globalData.Create<pce::Texture>(pce::Asset(dir / "wooded-heavy.png"));
        marsh = pce::globalData.Create<pce::Texture>(pce::Asset(dir / "marsh.png"));
    }
    [[nodiscard]] HandleOptional<Texture> ForFeature(const TerrainFeature feature) const {
        switch (feature) {
            case TerrainFeature::TERRAIN_FEATURE_GRASSLAND: return grassland;
            case TerrainFeature::TERRAIN_FEATURE_FIELD: return field;
            case TerrainFeature::TERRAIN_FEATURE_CITY: return city;
            case TerrainFeature::TERRAIN_FEATURE_VILLAGE: return village;
            case TerrainFeature::TERRAIN_FEATURE_WOODED_LIGHTLY: return wooded_lightly;
            case TerrainFeature::TERRAIN_FEATURE_WOODED_HEAVY: return wooded_heavy;
            case TerrainFeature::TERRAIN_FEATURE_MARSH: return marsh;
        }
        std::unreachable();
    }
};
struct TerrainFeatureTextureStack {
    TerrainFeatureTextures terrain_features_silhouettes { "terrain/terrain-silhouettes" };
    TerrainFeatureTextures terrain_features_icons { "terrain/terrain-icons" };
};
inline b8 SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Span<const Vertex> vertices, const Span<const i32> indices) { return SDL_RenderGeometry(renderer, texture, reinterpret_cast<const SDL_Vertex*>(vertices.data()), vertices.size(), indices.data(), indices.size()); }
inline b8 SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const Span<const Vertex> vertices) { return SDL_RenderGeometry(renderer, texture, reinterpret_cast<const SDL_Vertex*>(vertices.data()), vertices.size(), nullptr, 0); }

void AppendCountryBorders(HexState& hex_state, const CameraState& camera) {
    for (u32 i = 0; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (hex.owner.tag == CountryTag::TAG_NONE) { continue; }
        const int2 axial = hex_state.hex_map.IndexToAxial(i);
        const float2 world_center = HexAxialToWorld(axial);
        const float2 screen_center = camera.WorldToScreen(world_center);
        const ColorF color = static_cast<ColorF>(CountryTagToColor(hex.owner.tag));
        for (u32 s = 0; s < HEX_CORNERS; s++) {
            const int2 axial_neighbour = axial + HEX_AXIAL_NEIGHBOURS[s];
            if (!hex_state.hex_map.Contains(axial_neighbour) || hex_state.hex_map[axial_neighbour].owner.tag != hex.owner.tag) {
                const float2 local_angle_a = HEX_ANGLE[s];
                const float2 local_angle_b = HEX_ANGLE[(s + 1U) % HEX_CORNERS];
                const float2 screen_outer_a = screen_center + local_angle_a * float2(camera.scale);
                const float2 screen_outer_b = screen_center + local_angle_b * float2(camera.scale);
                const float2 screen_inner_a = screen_center + local_angle_a * float2(camera.scale * BORDER_INNER_RADIUS);
                const float2 screen_inner_b = screen_center + local_angle_b * float2(camera.scale * BORDER_INNER_RADIUS);
                hex_state.verts.EmplaceBack(screen_inner_a, color);
                hex_state.verts.EmplaceBack(screen_inner_b, color);
                hex_state.verts.EmplaceBack(screen_outer_b, color);
                hex_state.verts.EmplaceBack(screen_inner_a, color);
                hex_state.verts.EmplaceBack(screen_outer_b, color);
                hex_state.verts.EmplaceBack(screen_outer_a, color);

                const float2 screen_anchor = camera.WorldToScreen(world_center + (local_angle_a + local_angle_b) * float2(0.5F));
                const float2 local_edge = screen_inner_b - screen_inner_a;
                const f32 length_edge_squared = math::LengthSq(local_edge);
                if (length_edge_squared > 0.0001F) {
                    const float2 local_anchor = screen_anchor - screen_inner_a;
                    const f32 t = math::Clamp((local_anchor.x * local_edge.x + local_anchor.y * local_edge.y) / length_edge_squared, 0.0F, 1.0F);
                    const float2 screen_base_mid = screen_inner_a + local_edge * float2(t);
                    const float2 local_inward = (local_angle_a + local_angle_b) * float2(-0.5F);
                    const f32 local_inward_length = math::Hypot(local_inward);
                    if (local_inward_length > 0.0001F) {
                        const f32 length_edge = math::Sqrt(length_edge_squared);
                        const float2 local_edge_normalized = local_edge * float2(1.0F / length_edge);
                        const f32 width_teeth = math::Min(BORDER_TEETH_HALF * camera.scale, length_edge * 0.5F);
                        const float2 screen_base_a = screen_base_mid - local_edge_normalized * float2(width_teeth);
                        const float2 screen_base_b = screen_base_mid + local_edge_normalized * float2(width_teeth);

                        const float2 local_inward_normalized = local_inward * float2(1.0F / local_inward_length);
                        const f32 height_teeth = BORDER_TEETH_DEPTH * camera.scale;
                        const float2 screen_apex = screen_base_mid + local_inward_normalized * float2(height_teeth);
                        hex_state.verts.EmplaceBack(screen_base_a, color);
                        hex_state.verts.EmplaceBack(screen_base_b, color);
                        hex_state.verts.EmplaceBack(screen_apex, color);
                    }
                }
            }
        }
    }
}

[[nodiscard]] constexpr Color TerrainFeatureToTint(const TerrainFeature feature) {
    switch (feature) {
        case TerrainFeature::TERRAIN_FEATURE_CITY: return colors::COLOR_FEATURE_CITY;
        case TerrainFeature::TERRAIN_FEATURE_VILLAGE: return colors::COLOR_FEATURE_VILLAGE;
        case TerrainFeature::TERRAIN_FEATURE_WOODED_LIGHTLY: return colors::COLOR_FEATURE_WOODED_LIGHT;
        case TerrainFeature::TERRAIN_FEATURE_WOODED_HEAVY: return colors::COLOR_FEATURE_WOODED_HEAVY;
        case TerrainFeature::TERRAIN_FEATURE_FIELD: return colors::COLOR_FEATURE_FIELD;
        case TerrainFeature::TERRAIN_FEATURE_MARSH: return colors::COLOR_FEATURE_MARSH;
        default: return Color { 255U, 255U, 255U };
    }
}
void AppendTerrainFeatures(HexState& hex_state, const CameraState& camera) {
    SDL_Renderer* renderer = Singleton::Get<WindowState>().renderer;
    const TerrainFeatureTextureStack& stack = Singleton::Get<TerrainFeatureTextureStack>();
    const TerrainFeatureTextures& textures = TERRAIN_FEATURE_THEME == TerrainStyle::TERRAIN_STYLE_ICONS ? stack.terrain_features_icons : stack.terrain_features_silhouettes;
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (hex.terrain_feature != TerrainFeature::TERRAIN_FEATURE_GRASSLAND) {
            const HandleOptional<Texture> texture_handle = textures.ForFeature(hex.terrain_feature);
            if (texture_handle.IsValid()) {
                SDL_Texture* texture = pce::globalData[texture_handle.GetHandle()];
                const Color color = TerrainFeatureToTint(hex.terrain_feature);
                (void)SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
                const int2 axial = hex_state.hex_map.IndexToAxial(i);
                const float2 screen_local_jitter = HexTileJitter(axial) * float2 { FEATURE_POSITION_JITTER * camera.scale };
                const float2 screen = camera.WorldToScreen(HexAxialToWorld(axial)) + screen_local_jitter;
                const AABB screen_area = AABB::FromCenter(screen, float2 { camera.scale * 1.1F });
                (void)SDL_RenderTexture(renderer, texture, nullptr, screen_area);
            }
        }
    }
}

void AppendRoadMesh(HexState& hex_state, const CameraState& camera) {
    for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
        const Hex& hex = hex_state.hex_map.data[i];
        if (hex.roads.Any()) {
            const int2 axial = hex_state.hex_map.IndexToAxial(i);
            const float2 world = HexAxialToWorld(axial);
            const float2 screen = camera.WorldToScreen(world);
            const float2 screen_feature = screen + HexTileJitter(axial) * float2 { ROAD_CENTER_JITTER * camera.scale };
            for (u32 side = 0U; side < HEX_CORNERS; side++) {
                const RoadLevel road_level = static_cast<RoadLevel>(hex.roads.Test(side));
                if (road_level != RoadLevel::ROAD_LEVEL_NONE) {
                    const int2 axial_neighbour = axial + HEX_AXIAL_NEIGHBOURS[side];
                    const float2 world_neighbour = HexAxialToWorld(axial_neighbour);
                    const float2 screen_neighbour = camera.WorldToScreen(world_neighbour) + HexTileJitter(axial_neighbour) * float2 { ROAD_CENTER_JITTER * camera.scale };
                    const f32 screen_width = (road_level == RoadLevel::ROAD_LEVEL_MEDIUM ? ROAD_BIG_WIDTH : ROAD_WIDTH) * camera.scale;
                    VertObbAppend(hex_state.verts, OBB::BetweenPoints(screen_feature, screen_neighbour, screen_width), colors::COLOR_ROAD_GREY.WithAlpha(0.5F));
                }
            }
        }
    }
}

void AppendRiverMesh(HexState& hex_state, const CameraState& camera) {
    auto append_pass = [&](const f32 width, const ColorF color) {
        for (u32 i = 0U; i < hex_state.hex_map.Size(); i++) {
            const Hex& hex = hex_state.hex_map.data[i];
            if (hex.river_edges.Any()) {
                const int2 axial = hex_state.hex_map.IndexToAxial(i);
                const float2 world = HexAxialToWorld(axial);
                const float2 screen = camera.WorldToScreen(world);
                for (u32 side = 0U; side < HEX_CORNERS; side++) {
                    if (hex.river_edges.Test(side)) {
                        const float2 screen_corner_a = screen + HEX_ANGLE[side % HEX_CORNERS] * float2 { camera.scale };
                        const float2 screen_corner_b = screen + HEX_ANGLE[(side + 1U) % HEX_CORNERS] * float2 { camera.scale };
                        VertObbAppend(hex_state.verts, OBB::BetweenPoints(screen_corner_a, screen_corner_b, width), color);
                        VertAabbAppend(hex_state.verts, AABB::FromCenter(screen_corner_a, float2 { 10.0F }), colors::COLOR_BLUE);
                        VertAabbAppend(hex_state.verts, AABB::FromCenter(screen_corner_b, float2 { 10.0F }), colors::COLOR_RED);
                        int2 axial_neighbour = axial + HEX_AXIAL_NEIGHBOURS[side];
                        float2 screen_neighbour = camera.WorldToScreen(HexAxialToWorld(axial_neighbour));
                        VertObbAppend(hex_state.verts, OBB::BetweenPoints(screen_neighbour, screen, 10.0f), colors::COLOR_BROWN);
                    }
                }
            }
        }
    };

    append_pass((RIVER_WIDTH + RIVER_CASING_EXTRA) * camera.scale, colors::COLOR_RIVER_DEEP_BLUE);
    append_pass(RIVER_WIDTH * camera.scale, colors::COLOR_RIVER_BLUE);
    append_pass(RIVER_HIGHLIGHT_WIDTH * camera.scale, colors::COLOR_RIVER_HIGHLIGHT_BLUE);
}
}
