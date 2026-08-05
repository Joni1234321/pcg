# font tracker

Adjust **letter-spacing (tracking)** and export a patched `.ttf`.

SDL_ttf has no letter-spacing API — the complete set of font setters is `Size`,
`SizeDPI`, `Style`, `Outline`, `Hinting`, `SDF`, `WrapAlignment`, `LineSkip`,
`Kerning` (a bool), `Direction`, `Script`, `Language`. So instead of doing
per-glyph layout in the engine, this bakes the spacing into the font file.

Every glyph's advance width lives in the font's `hmtx` table. Subtract a
constant from all of them and text renders tighter — permanently, in the asset.

## Why this beats doing it in the engine

FreeType reads `hmtx`, so a pre-tracked font flows through the existing text
path untouched:

- `TTF_Text` still works — no per-glyph draw loop
- `TTF_GetTextSize` still measures correctly, so `r_ui_node.cppm` layout stays honest
- `TTF_SetTextWrapWidth` still wraps correctly
- still one `TTF_DrawRendererText` call per string

Glyph *shapes* are untouched too, so you don't get the stretched look that
`TTF_SetFontSizeDPI` with mismatched h/v DPI produces.

## Setup

```powershell
cd tools/font-tracker
npm install
```

## Run

```powershell
npm start
```

Serves at `http://localhost:8080`. Opening [index.html](index.html) from
`file://` also works — everything runs client-side — but `npm start` is safer.

## Usage

1. Drop in a font. For this repo: `assets/Titillium_Web/font.ttf`.
2. Drag the tracking slider. Negative tightens.
3. Set **Preview size** to the size you actually render at, and **Field width
   guide** to the pixel width of the field you're trying to fit. The dashed red
   line shows the limit; the note under the previews tells you whether you made it.
4. **Download patched font**, drop it in `assets/`, and point
   `FontCollection::SetFontFile` at it.

## Units

Tracking is in **1/1000 em**, the typographic standard. It's size-independent —
`-20` removes 2% of the em from every glyph, whether you render at 12px or 72px.
Converted to font units as `round(perMille / 1000 * unitsPerEm)`.

The slider spans `-1000`..`+1000`, and the number box takes anything at all —
the slider just pins to whichever end you ran past. Measured on
`assets/Titillium_Web/font.ttf` (1000 upm, 410 glyphs) against the string
`Ranked Flex  6/7/7  188 CS (4.7)`:

| Value  | Width @16px | vs original | Clamped glyphs | Look |
|--------|-------------|-------------|----------------|------|
| `0`    | 216.9 px    | —           | 0 | Untouched. |
| `-20`  | 206.6 px    | −4.7%       | 0 | Dense UI. The usual choice for stat rows. |
| `-60`  | 186.2 px    | −14.2%      | 0 | Very tight, still readable. |
| `-120` | 155.4 px    | −28.3%      | 0 | Extreme. Letters touch. |
| `-250` | 93.1 px     | −57.1%      | 55 | Heavy overlap. |
| `-400` | 42.2 px     | −80.5%      | 111 | Mostly a stack. |
| `+50`  | 242.5 px    | +11.8%      | 0 | Loosened, for airy headings. |

Around **−170** the narrowest glyph in Titillium Web (170 units) reaches zero
advance and clamps — a glyph can't advance backwards. Past that the tool still
exports fine, but spacing stops being uniform: clamped glyphs sit directly on
top of the next one while wider glyphs keep advancing. The page warns you when
this starts and shows the narrowest surviving advance.

For legible text, `-120` is roughly the floor on this font. Beyond that you want
a genuinely condensed typeface rather than a tracked one — though the range is
open if you're after a deliberately collapsed/overlapping display effect.

## Caveats

- **Only the advance shrinks**, so space comes off each glyph's right side.
  Since every glyph gets the same treatment, spacing *between* glyphs is
  uniformly tightened; only the string's trailing edge differs from a true
  symmetric adjustment, which is invisible in practice.
- **Zero-advance glyphs are skipped** — those are combining marks, and moving
  them would detach accents from their base letters.
- **opentype.js re-serialises the font** rather than patching bytes, so tables
  it doesn't round-trip (`GPOS`/`GSUB`, hinting) can be lost. Irrelevant for a
  game UI in Latin script; if you care, use the lossless path below.
- **`.otf` (CFF outlines)** round-trips less reliably than TrueType. Check the
  export, or use fontTools.

## Lossless alternative

Use the page to find the value by eye, then generate the shipping asset with
fontTools — it patches `hmtx` in place and leaves every other table
byte-identical:

```powershell
pip install fonttools
```

```python
from fontTools.ttLib import TTFont

TRACKING = -20                       # 1/1000 em, from the page

f = TTFont("font.ttf")
delta = round(TRACKING / 1000 * f["head"].unitsPerEm)
hmtx = f["hmtx"]
for name in f.getGlyphOrder():
    aw, lsb = hmtx[name]
    if aw:                           # skip combining marks
        hmtx[name] = (max(0, aw + delta), lsb)
f["hhea"].advanceWidthMax = max(hmtx[n][0] for n in f.getGlyphOrder())
f.save("font-tight.ttf")
```

## Related

Vertical spacing needs none of this — `TTF_SetFontLineSkip(font, n)` sets line
spacing directly at runtime, and wrapped `TTF_Text` picks it up automatically.
