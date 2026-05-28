# silhouette maker

Drop any image (weapon photo, vehicle, soldier, whatever you found on Google
Images), get a clean silhouette PNG/JPG at any size and color. Pure client-side
— images never leave your browser.

## Setup

```powershell
cd tools/silhouette-maker
npm install
```

(Only JSZip is installed, used for batch zip downloads.)

## Run

```powershell
npm start
```

…then open `http://localhost:8080`. Or just open [index.html](index.html).

## Workflow

1. Find a weapon/vehicle photo on Google Images (look for ones with a plain
   white/gray/black background — they convert cleanest).
2. Either:
   - **Right-click → Copy image**, then click the dropzone and press
     <kbd>Ctrl</kbd>+<kbd>V</kbd>.
   - **Right-click → Save image as…**, then drag the file onto the dropzone.
   - Drop multiple files at once for batch conversion.
   - Paste a URL (only works for sites that allow cross-origin loads).
3. Tweak settings on the right — preview updates instantly.
4. Hit **Download all** — one image downloads directly, multiple zip up.

## Algorithms

- **Remove background by color** (default): samples the background color from
  the corners (or you pick one) and removes everything close to it. Best for
  product photos and stock images.
- **Brightness threshold**: anything darker than the threshold becomes the
  silhouette. Great for icon-like art or when the subject is much darker than
  the background.
- **Use existing alpha**: pass-through for PNGs that already have transparency
  — useful to just recolor + resize.

### Tuning tips

- **Threshold** = how close to the background a pixel has to be to count as
  background. Higher = more aggressive removal (eats into subject); lower =
  leaves a halo.
- **Edge softness** = anti-aliasing band around the edge. 0 = hard mask,
  10–20 gives nice smooth edges.
- **Fit mode = Trim to silhouette bbox**: auto-crops empty margins from the
  source so the silhouette fills the output (great for consistent counter
  textures).
- For best results on tricky photos, open the source in any editor and erase
  the background to transparency first, then use **Use existing alpha** here
  for resize + recolor.
