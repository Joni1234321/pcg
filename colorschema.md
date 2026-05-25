# Color scheme ideas

A scratchpad of palette ideas explored during the hex-map polish session.
The currently-shipped schemes live in `src/g_hex.hpp` as
`enum class TerrainScheme`. Switch between them by changing the
`TERRAIN_SCHEME` `constexpr` at the top of that header.

## Terrain palettes

### `CIV_VIBRANT` (default)
The original saturated palette. Reads at a glance, good for prototyping.

| terrain      | rgb              | notes                            |
| ------------ | ---------------- | -------------------------------- |
| DEEP_OCEAN   | `20, 60, 120`    | almost-navy                      |
| OCEAN        | `50, 100, 180`   | classic cobalt                   |
| BEACH        | KHAKI            | warm contrast against ocean      |
| GRASS        | `100, 190, 80`   | grassland green                  |
| FOREST       | FOREST_GREEN     | dark woods                       |
| MOUNTAIN     | GRAY             | neutral rock                     |
| SNOW         | WHITE            | flat white peaks                 |

### `SLATE_TABLE`
Cool, low-saturation. Reads like a tabletop wargame printed on slate.
Pairs well with strong country overlays in saturated red / blue.

| terrain      | rgb              |
| ------------ | ---------------- |
| DEEP_OCEAN   | `28, 38, 55`     |
| OCEAN        | `52, 72, 96`     |
| BEACH        | `156, 144, 110`  |
| GRASS        | `102, 122, 86`   |
| FOREST       | `62, 84, 60`     |
| MOUNTAIN     | `92, 92, 100`    |
| SNOW         | `198, 204, 212`  |

### `HOI4_PAPER`
Warm parchment / political-map look. Lower contrast between biomes so the
country overlays carry the visual weight.

| terrain      | rgb              |
| ------------ | ---------------- |
| DEEP_OCEAN   | `88, 112, 142`   |
| OCEAN        | `130, 158, 184`  |
| BEACH        | `220, 200, 158`  |
| GRASS        | `178, 184, 132`  |
| FOREST       | `128, 144, 96`   |
| MOUNTAIN     | `152, 138, 116`  |
| SNOW         | `232, 226, 210`  |

## Other palettes explored but not kept

These were tried during the iteration session and are worth revisiting:

- **PASTEL_HIGHLAND** — soft pastels, like a kid's atlas. Friendly but
  hard to tell biomes apart at small zoom.
- **PARCHMENT_MAP** — sepia base with hand-drawn feel. Good for menus,
  bad for gameplay legibility.
- **ARCTIC_PAPER** — near-monochrome whites/blues with thin warm
  accents. Beautiful but unreadable without strong overlays.
- **MUTED_TOPO** — desaturated topographic map. Close to `HOI4_PAPER`
  but with stronger elevation cues; ocean basically grey-blue.
- **BLUEPRINT** — true blueprint look: dark navy background, cyan/white
  line strokes for terrain. Needs a non-fill render mode to work.
- **SEPIA_RECON** — old aerial-recon photo. All warm browns; nice in
  isolation, washes out against red/blue countries.

## Background

The map backdrop is set in `g_hex.cpp::arcade::RunHex()` to a deep blue
`Color::FromHsl(210, 0.55, 0.20)`. Earlier experiments:

- `LIGHT_SKY_BLUE` — too light, hexes melted into it.
- `HSL(180, 0.5, 0.2)` — teal; reads as water everywhere, fights the
  ocean tiles.
- `HSL(40, 0.5, 0.2)` — warm brown; "poop" per design review, rejected.
- `HSL(210, 0.55, 0.20)` — current pick; deep enough to frame any
  terrain palette, blue enough to feel "map".

## Country colors

Defined in `u_colors.hpp` via `Color::FromHsl` so luminance is shared:

- GER — H 90, S 0.20  (muted olive)
- SOV — H  2, S 0.60  (warm red)
- USA — H 218, S 0.60 (cool blue)

Shared `COUNTRY_LUMINANCE` keeps them readable when overlaid as a
translucent territory band.

## Border styling ideas (for future work)

These design notes came out of the Civ6-style border experiment:

- Two-quad radial tessellation gives a `pow(t, gamma)` alpha fade from
  outer edge into the territory; `gamma > 1` keeps the band bold then
  drops fast, `gamma < 1` fades immediately.
- Mitered joins across same-country hexes: build a corner→edges spatial
  hash and offset each endpoint along the bisector of the two inward
  normals, scaled by `1 / cos(theta/2)`. Clamp at 4x to avoid spikes on
  sharp turns.
- Outer-vs-inner placement: anchor the strip to the lattice radius (1.0)
  not the shrunk fill so `hex_fill_scale` and the border are
  independent. Then `border_outset` extends beyond the lattice edge and
  `border_depth` extends inside.

## Fog of war ideas

- BFS from every player-owned hex; per non-player tile the fog alpha is
  step-shaped: `0` within `fog_radius`, half-strength at exactly
  `fog_radius + 1`, full beyond. Single soft ring sells the boundary
  well enough without a real gradient.
- For undiscovered terrain (`country_tag == NONE`) the same rule
  applies; can later split into "never seen" vs "remembered" by
  storing a `discovered` bit and rendering with two alpha tiers.

## Map noise (cities / villages / roads / rivers)

Cheap to add and visually huge. Hash-based, deterministic:

- `AxialHash(axial)` for per-hex features (cities/villages on buildable
  terrain).
- `EdgeHash(a, b)` for per-edge features. Threshold the low bits:
  `& 0x07 == 0` for roads (between buildable hexes),
  `& 0x1F == 1` for rivers (along the edge of two land hexes).
- Render with a thick-line segment helper (`SegmentAppend`) using the
  edge perpendicular for offset.
