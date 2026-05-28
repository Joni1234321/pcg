# milsymbol downloader

A tiny standalone tool to render NATO military symbols (APP-6 / MIL-STD-2525) from
SIDC codes and export them as **SVG**, **PNG**, or **JPG** at any dimensions —
useful for generating test textures for the game.

## Setup

```powershell
cd tools/milsymbol-downloader
npm install
```

That pulls `milsymbol` and `jszip` into `node_modules/` — no manual downloads
and no CDN needed.

## Run

Two options:

1. **Just open the file**: double-click [index.html](index.html). It loads the
   libraries from the local `node_modules/` folder, so it works fully offline.
2. **Serve it** (recommended — some browsers block `file://` for blob URLs):

   ```powershell
   npm start
   ```

   This starts a small static server on `http://localhost:8080` and opens it.

## Usage

1. Paste one **SIDC** per line (15-char 2525C or 20-char 2525D / APP-6D).
2. Pick format, width, height, symbol size, padding, etc.
3. Hit **Download all**:
   - 1 code → single file download
   - multiple codes → a `.zip` of all rendered files

Filenames are the SIDC with non-filesystem chars replaced by `_`, e.g.
`SFGPUCI-----E.png`.

## SIDC quick reference

Position 1-2 of the 15-char code controls affiliation/standard identity:

| Code | Affiliation | Frame color |
|------|-------------|-------------|
| `SF` | Friend      | Blue        |
| `SH` | Hostile     | Red         |
| `SN` | Neutral     | Green       |
| `SU` | Unknown     | Yellow      |

Examples:
- `SFGPUCI-----E` — friendly infantry
- `SHGPUCA-----E` — hostile armor
- `SFGPUCF-----E` — friendly field artillery
- `SFAPMFF-----E` — friendly fighter aircraft

See [milsymbol docs](https://www.spatialillusions.com/milsymbol/docs/index.html)
for the full SIDC grammar.
