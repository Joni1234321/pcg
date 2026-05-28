# silhouette finder (wargame)

Search realistic military silhouettes on [Wikimedia Commons](https://commons.wikimedia.org)
(SVG only), recolor them, and export at any size as SVG / PNG / JPG.

Designed for wargame textures where you want recognizable real weapons,
vehicles, aircraft, and ships — not stylized icons.

## Setup

```powershell
cd tools/silhouette-finder
npm install
```

Only JSZip is installed locally; all SVG files come from Wikimedia Commons over
HTTPS (the API supports CORS).

## Run

```powershell
npm start
```

This serves the app on `http://localhost:8080`. You can also just open
[index.html](index.html), but Commons CORS is more reliable through a real
HTTP origin.

## How it works

- **Search**: queries the Commons MediaWiki API for files matching your terms,
  restricted to SVGs. A "silhouette" / "profile" / "outline" suffix is added
  by default to bias toward what wargames need; switch to *No suffix* for a
  raw search.
- **Preview**: thumbnails come from the Commons thumbnail service (so the
  preview tiles aren't tinted yet).
- **Selection**: click any tile to add it to *Selected*. Tooltip shows
  license / author / source URL.
- **Recoloring**: when *Keep original* is unchecked, every solid `fill` /
  `stroke` (attribute or inline style) in the SVG is replaced with your chosen
  tint color. Works perfectly on flat silhouettes. Skips `none`, `transparent`,
  and gradient/pattern refs (`url(#…)`).
- **Export**: same pipeline as the other tools — fits the SVG into a target
  W×H canvas with optional background, exports as SVG / PNG (transparent) /
  JPG (background color).
- **Attribution**: the zip download includes `ATTRIBUTION.txt` listing every
  file's license, author, and Commons page URL. Keep this with your assets
  to comply with CC-BY / CC-BY-SA.

## Recommended search terms

| Want | Try |
|------|-----|
| Small arms | `rifle silhouette`, `AK-47 silhouette`, `M16 outline`, `handgun silhouette` |
| Tanks / AFVs | `tank silhouette`, `M1 Abrams profile`, `T-72 profile`, `IFV silhouette` |
| Aircraft | `F-16 silhouette`, `Su-27 silhouette`, `helicopter silhouette`, `bomber silhouette` |
| Ships | `destroyer silhouette`, `aircraft carrier silhouette`, `frigate profile` |
| Troops | `soldier silhouette`, `infantry silhouette`, `helmet silhouette` |
| Equipment | `mortar silhouette`, `artillery silhouette`, `radar silhouette` |

## Limitations

- Wikimedia Commons quality varies — preview before adding to selection.
- Recoloring is heuristic (regex/DOM walk). If a SVG uses CSS classes or
  complex gradients, the result may be partially recolored. Use *Keep
  original* and recolor downstream if needed.
- The Commons API can be slow; *Load more* paginates 30 results at a time.
