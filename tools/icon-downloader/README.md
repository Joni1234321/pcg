# icon downloader

Search and export thousands of free SVG icons (weapon silhouettes, units,
UI glyphs, etc.) for game textures. Uses the free
[Iconify API](https://iconify.design) — no API key, no package downloads for
icons — and lets you choose color, size, format.

Great for placeholder/test art you can tint at runtime.

## Setup

```powershell
cd tools/icon-downloader
npm install
```

(Only JSZip is installed locally — for bundling multi-icon zip exports.
Icons themselves come from the Iconify API over HTTPS.)

## Run

Open [index.html](index.html) directly, **or**:

```powershell
npm start
```

This serves it at `http://localhost:8080`. Some browsers block `fetch` from
`file://` URLs, so `npm start` is the safer option.

## Recommended icon collections

| Prefix             | Set                       | License        | Notes                                     |
|--------------------|---------------------------|----------------|-------------------------------------------|
| `game-icons`       | game-icons.net            | CC-BY 3.0      | 4000+ game-styled silhouettes — weapons, units, magic, fantasy. Best for tinting. |
| `lucide`           | Lucide                    | ISC            | Clean modern line icons.                  |
| `ph`               | Phosphor                  | MIT            | Versatile UI icon set.                    |
| `mdi`              | Material Design Icons     | Apache 2.0     | Huge generic set.                         |
| `tabler`           | Tabler                    | MIT            | Outline UI icons.                         |
| `fa6-solid`        | Font Awesome 6 Solid      | CC-BY 4.0      | Familiar solid glyphs.                    |
| `material-symbols` | Material Symbols          | Apache 2.0     | Google's variable-style icons.            |

Iconify aggregates 200+ sets — the dropdown only lists the most useful ones,
but you can switch to **All collections** in the UI.

## Usage

1. Type a query (e.g. `sword`, `rifle`, `helmet`, `tank`, `soldier`, `spell`).
2. Click icons in the grid to add them to **Selected icons**.
3. Choose format / size / color / background.
4. **Download all** — single icon → direct file, multiple → a `.zip` that
   also contains an `ATTRIBUTION.txt` whenever any icon comes from a set
   that requires attribution (e.g. game-icons CC-BY 3.0).

### Tips

- **White silhouettes for tinting**: keep the default — Icon color `#ffffff`,
  background Transparent, format PNG. Then multiply/tint in-engine.
- **Custom tint baked in**: pick any color in **Icon color**; preview updates
  instantly.
- The **Keep original** checkbox preserves multi-color icons (rare in
  game-icons, common in Phosphor/Material).
- Iconify's color parameter only recolors monochrome icons (icons declared as
  single-color in their source). Most game-icons icons are monochrome.

## Licenses

Check each icon set's license — most require attribution somewhere in your
product credits. The exported `ATTRIBUTION.txt` lists the sets used in the
batch.
