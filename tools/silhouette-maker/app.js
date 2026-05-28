/* silhouette-maker -- drop images, get clean silhouettes (background removal + tint) */
(function () {
    "use strict";

    const $ = (id) => document.getElementById(id);

    const els = {
        dropzone: $("dropzone"),
        filePicker: $("filePicker"),
        urlInput: $("urlInput"),
        urlAdd: $("urlAdd"),

        algo: $("algo"),
        bgSample: $("bgSample"),
        bgPick: $("bgPick"),
        threshold: $("threshold"),
        thresholdVal: $("thresholdVal"),
        softness: $("softness"),
        softnessVal: $("softnessVal"),
        brightness: $("brightness"),
        brightnessVal: $("brightnessVal"),
        silColor: $("silColor"),
        outBg: $("outBg"),
        outBgTransparent: $("outBgTransparent"),
        outWidth: $("outWidth"),
        outHeight: $("outHeight"),
        fitMode: $("fitMode"),
        padPct: $("padPct"),
        format: $("format"),
        suffix: $("suffix"),
        invertMask: $("invertMask"),

        rotation: $("rotation"),
        rotationVal: $("rotationVal"),
        scale: $("scale"),
        scaleVal: $("scaleVal"),
        offsetX: $("offsetX"),
        offsetXVal: $("offsetXVal"),
        offsetY: $("offsetY"),
        offsetYVal: $("offsetYVal"),
        flipH: $("flipH"),
        flipV: $("flipV"),
        dilate: $("dilate"),
        dilateVal: $("dilateVal"),
        feather: $("feather"),
        featherVal: $("featherVal"),
        outlineW: $("outlineW"),
        outlineWVal: $("outlineWVal"),
        outlineColor: $("outlineColor"),
        shadowBlur: $("shadowBlur"),
        shadowBlurVal: $("shadowBlurVal"),
        shadowX: $("shadowX"),
        shadowXVal: $("shadowXVal"),
        shadowY: $("shadowY"),
        shadowYVal: $("shadowYVal"),
        shadowColor: $("shadowColor"),
        shadowAlpha: $("shadowAlpha"),
        shadowAlphaVal: $("shadowAlphaVal"),
        resetTransform: $("resetTransform"),
        resetMask: $("resetMask"),
        resetOutline: $("resetOutline"),
        resetShadow: $("resetShadow"),
        resetAll: $("resetAll"),

        fxTransform: $("fxTransform"),
        fxMask: $("fxMask"),
        fxOutline: $("fxOutline"),
        fxShadow: $("fxShadow"),

        pageBg: $("pageBg"),
        pageBgCheck: $("pageBgCheck"),

        presetSave: $("presetSave"),
        presetLoadBtn: $("presetLoadBtn"),
        presetFile: $("presetFile"),
        presetCopy: $("presetCopy"),
        presetApply: $("presetApply"),
        presetText: $("presetText"),
        presetStatus: $("presetStatus"),

        previews: $("previews"),
        downloadAll: $("downloadAll"),
        clearAll: $("clearAll"),
        status: $("status"),
    };

    // Each item: { id, name, imgEl, sourceUrl, tile, srcCanvas, outCanvas }
    const items = [];
    let nextId = 1;

    // ─── Helpers ─────────────────────────────────────────────────────────
    function clampInt(v, lo, hi, def) {
        const n = parseInt(v, 10);
        if (!Number.isFinite(n)) return def;
        return Math.min(hi, Math.max(lo, n));
    }
    function clampFloat(v, lo, hi, def) {
        const n = parseFloat(v);
        if (!Number.isFinite(n)) return def;
        return Math.min(hi, Math.max(lo, n));
    }
    function hexToRgb(hex) {
        const m = /^#?([0-9a-f]{6})$/i.exec(hex);
        if (!m) return [255, 255, 255];
        const n = parseInt(m[1], 16);
        return [(n >> 16) & 255, (n >> 8) & 255, n & 255];
    }
    function sanitizeFileName(name) {
        return (name || "image").replace(/[^A-Za-z0-9._-]/g, "_");
    }
    function baseName(name) {
        return (name || "image").replace(/\.[^.]+$/, "");
    }

    // ─── Image loading ───────────────────────────────────────────────────
    function loadFile(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = () => {
                const img = new Image();
                img.onload = () => resolve({ img, name: file.name, blob: file });
                img.onerror = () => reject(new Error("Image decode failed"));
                img.src = reader.result;
            };
            reader.onerror = () => reject(new Error("File read failed"));
            reader.readAsDataURL(file);
        });
    }

    function loadUrl(url) {
        return new Promise((resolve, reject) => {
            const img = new Image();
            img.crossOrigin = "anonymous";
            img.onload = async () => {
                const name = (url.split("/").pop() || "image").split("?")[0] || "image";
                let blob = null;
                try {
                    const r = await fetch(url, { mode: "cors" });
                    if (r.ok) blob = await r.blob();
                } catch { /* ignore — we still have the decoded image */ }
                resolve({ img, name, blob });
            };
            img.onerror = () => reject(new Error(
                "Failed to load. The site likely blocks cross-origin loads. " +
                "Right-click the image, 'Save image as…', then drop the file here."
            ));
            img.src = url;
        });
    }

    function addImage(img, name, originalBlob) {
        const id = nextId++;
        const srcCanvas = document.createElement("canvas");
        srcCanvas.width = img.naturalWidth;
        srcCanvas.height = img.naturalHeight;
        const sctx = srcCanvas.getContext("2d");
        sctx.drawImage(img, 0, 0);

        const item = { id, name, sourceImg: img, srcCanvas, outCanvas: null, tile: null,
                       originalBlob: originalBlob || null };
        items.push(item);
        renderTile(item);
        rebuild(item);
    }

    // Decode a Blob into an <img> element.
    function blobToImage(blob) {
        return new Promise((resolve, reject) => {
            const url = URL.createObjectURL(blob);
            const img = new Image();
            img.onload = () => { resolve(img); };
            img.onerror = () => { URL.revokeObjectURL(url); reject(new Error("Decode failed")); };
            img.src = url;
        });
    }

    // Load a previously-exported bundle: applies preset.json and re-adds originals/*.
    async function loadBundleZip(file) {
        if (typeof JSZip === "undefined") throw new Error("JSZip not loaded");
        const zip = await JSZip.loadAsync(file);

        // 1) Apply preset.json if present.
        const presetEntry = zip.file(/^preset\.json$/i)[0];
        let appliedCount = 0;
        if (presetEntry) {
            try {
                const text = await presetEntry.async("string");
                const obj = JSON.parse(text);
                appliedCount = applySettings(obj);
                if (els.presetText) els.presetText.value = JSON.stringify(obj, null, 2);
            } catch (e) {
                setStatus("Bundle preset.json parse error: " + (e.message || e));
            }
        }

        // 2) Re-add every file under originals/ as a source image.
        const originals = zip.file(/^originals\//i);
        let imgCount = 0;
        for (const entry of originals) {
            if (entry.dir) continue;
            try {
                const blob = await entry.async("blob");
                const name = entry.name.replace(/^originals\//i, "");
                // JSZip blobs may not have a useful mime; sniff by extension.
                const typed = new Blob([blob], { type: mimeFromName(name) || blob.type || "image/png" });
                const img = await blobToImage(typed);
                addImage(img, name, typed);
                imgCount++;
            } catch (e) {
                console.warn("Failed to load original", entry.name, e);
            }
        }

        const parts = [];
        if (imgCount) parts.push(`${imgCount} original image(s)`);
        if (appliedCount) parts.push(`${appliedCount} settings`);
        setStatus(parts.length ? `Bundle loaded: ${parts.join(" + ")}.` : "Bundle loaded (no usable content).");
    }

    function mimeFromName(name) {
        const ext = (name.split(".").pop() || "").toLowerCase();
        return {
            png: "image/png", jpg: "image/jpeg", jpeg: "image/jpeg",
            webp: "image/webp", gif: "image/gif", bmp: "image/bmp", svg: "image/svg+xml",
        }[ext] || "";
    }

    // ─── Silhouette algorithms ───────────────────────────────────────────
    /**
     * Build the silhouette canvas for an item using current settings.
     */
    function buildSilhouette(item) {
        const algo = els.algo.value;
        const srcW = item.srcCanvas.width;
        const srcH = item.srcCanvas.height;
        const srcImg = item.srcCanvas.getContext("2d").getImageData(0, 0, srcW, srcH);
        const src = srcImg.data;

        // Build a mask: alpha 0..255 for each src pixel (foreground likelihood)
        let mask = new Uint8ClampedArray(srcW * srcH);

        if (algo === "alpha") {
            for (let i = 0, p = 0; i < src.length; i += 4, p++) mask[p] = src[i + 3];
        } else if (algo === "brightness") {
            const thr = clampInt(els.brightness.value, 0, 255, 160);
            const soft = clampInt(els.softness.value, 0, 80, 0);
            for (let i = 0, p = 0; i < src.length; i += 4, p++) {
                const a = src[i + 3];
                if (a === 0) { mask[p] = 0; continue; }
                const lum = 0.2126 * src[i] + 0.7152 * src[i + 1] + 0.0722 * src[i + 2];
                // Dark = silhouette: closer to 0 = stronger foreground
                const dist = thr - lum;
                if (soft <= 0) mask[p] = dist > 0 ? 255 : 0;
                else mask[p] = Math.max(0, Math.min(255, ((dist / soft) * 255) | 0));
            }
        } else {
            // bg-color: remove pixels close to background color
            const bg = sampleBgColor(item.srcCanvas);
            const thr = clampInt(els.threshold.value, 0, 255, 40);
            const soft = clampInt(els.softness.value, 0, 80, 0);
            for (let i = 0, p = 0; i < src.length; i += 4, p++) {
                const a = src[i + 3];
                if (a === 0) { mask[p] = 0; continue; }
                const dr = src[i]     - bg[0];
                const dg = src[i + 1] - bg[1];
                const db = src[i + 2] - bg[2];
                // perceptual-ish distance
                const dist = Math.sqrt(dr * dr * 0.299 + dg * dg * 0.587 + db * db * 0.114);
                if (soft <= 0) {
                    mask[p] = dist > thr ? 255 : 0;
                } else {
                    const t = (dist - thr) / soft;
                    mask[p] = Math.max(0, Math.min(255, (t * 255) | 0));
                }
            }
        }

        // Invert mask if requested (only when Mask edit section is enabled).
        if (els.fxMask.checked && els.invertMask.checked) {
            for (let p = 0; p < mask.length; p++) mask[p] = 255 - mask[p];
        }

        // Dilate (positive) or erode (negative) the mask.
        const dilatePx = els.fxMask.checked ? clampInt(els.dilate.value, -20, 20, 0) : 0;
        if (dilatePx !== 0) {
            mask = morphology(mask, srcW, srcH, dilatePx);
        }

        // Build a silhouette image: tint color where mask>0, transparent elsewhere.
        const [sr, sg, sb] = hexToRgb(els.silColor.value);
        const silImg = new ImageData(srcW, srcH);
        const sil = silImg.data;
        for (let p = 0, i = 0; p < mask.length; p++, i += 4) {
            const m = mask[p];
            if (m === 0) { sil[i] = 0; sil[i + 1] = 0; sil[i + 2] = 0; sil[i + 3] = 0; }
            else { sil[i] = sr; sil[i + 1] = sg; sil[i + 2] = sb; sil[i + 3] = m; }
        }

        // Compose into the output canvas at the requested size/fit/padding/bg.
        const outW = clampInt(els.outWidth.value, 8, 4096, 512);
        const outH = clampInt(els.outHeight.value, 8, 4096, 512);
        const out = document.createElement("canvas");
        out.width = outW; out.height = outH;
        const octx = out.getContext("2d");

        if (!els.outBgTransparent.checked) {
            octx.fillStyle = els.outBg.value;
            octx.fillRect(0, 0, outW, outH);
        }

        // Stage canvas holds the silhouette at native resolution.
        let stage = document.createElement("canvas");
        stage.width = srcW; stage.height = srcH;
        stage.getContext("2d").putImageData(silImg, 0, 0);

        // Apply feather (blur on the silhouette alpha).
        const feather = els.fxMask.checked ? clampFloat(els.feather.value, 0, 20, 0) : 0;
        if (feather > 0) {
            const f = document.createElement("canvas");
            f.width = srcW; f.height = srcH;
            const fctx = f.getContext("2d");
            fctx.filter = `blur(${feather}px)`;
            fctx.drawImage(stage, 0, 0);
            stage = f;
        }

        // For crop mode, trim stage to silhouette bbox.
        if (els.fitMode.value === "crop") {
            const bbox = silhouetteBBox(mask, srcW, srcH);
            if (bbox) {
                const bw = bbox.x2 - bbox.x1;
                const bh = bbox.y2 - bbox.y1;
                if (bw > 0 && bh > 0) {
                    const trimmed = document.createElement("canvas");
                    trimmed.width = bw; trimmed.height = bh;
                    trimmed.getContext("2d").drawImage(stage, bbox.x1, bbox.y1, bw, bh, 0, 0, bw, bh);
                    stage = trimmed;
                }
            }
        }

        // Determine draw size inside padding.
        const fitMode = els.fitMode.value;
        const padPct = clampFloat(els.padPct.value, 0, 40, 0) / 100;
        const padX = outW * padPct;
        const padY = outH * padPct;
        const innerW = Math.max(1, outW - padX * 2);
        const innerH = Math.max(1, outH - padY * 2);

        let dw, dh;
        if (fitMode === "stretch") {
            dw = innerW; dh = innerH;
        } else {
            const s = Math.min(innerW / stage.width, innerH / stage.height);
            dw = stage.width * s;
            dh = stage.height * s;
        }

        // Transform inputs (gated by transform card).
        const tEnabled = els.fxTransform.checked;
        const rot = tEnabled ? clampFloat(els.rotation.value, -180, 180, 0) * Math.PI / 180 : 0;
        const scl = tEnabled ? clampFloat(els.scale.value, 10, 300, 100) / 100 : 1;
        const offX = tEnabled ? clampFloat(els.offsetX.value, -50, 50, 0) / 100 * outW : 0;
        const offY = tEnabled ? clampFloat(els.offsetY.value, -50, 50, 0) / 100 * outH : 0;
        const fH = (tEnabled && els.flipH.checked) ? -1 : 1;
        const fV = (tEnabled && els.flipV.checked) ? -1 : 1;

        // Build optional outline canvas (a ring of outlineColor around the silhouette).
        const outlineW = els.fxOutline.checked ? clampFloat(els.outlineW.value, 0, 20, 0) : 0;
        let outlineCanvas = null;
        if (outlineW > 0) outlineCanvas = buildOutlineCanvas(stage, outlineW, els.outlineColor.value);

        // Render the silhouette + outline into a transparent layer canvas at output size,
        // applying transform once. Then composite that layer onto octx with a drop shadow.
        const layer = document.createElement("canvas");
        layer.width = outW; layer.height = outH;
        const lctx = layer.getContext("2d");

        const cx = outW / 2 + offX;
        const cy = outH / 2 + offY;
        lctx.translate(cx, cy);
        lctx.rotate(rot);
        lctx.scale(scl * fH, scl * fV);
        if (outlineCanvas) {
            const ow = dw * (outlineCanvas.width / stage.width);
            const oh = dh * (outlineCanvas.height / stage.height);
            lctx.drawImage(outlineCanvas, -ow / 2, -oh / 2, ow, oh);
        }
        lctx.drawImage(stage, -dw / 2, -dh / 2, dw, dh);

        // Drop shadow (gated).
        const shEnabled = els.fxShadow.checked;
        const shBlur = shEnabled ? clampFloat(els.shadowBlur.value, 0, 40, 0) : 0;
        const shX = shEnabled ? clampFloat(els.shadowX.value, -40, 40, 0) : 0;
        const shY = shEnabled ? clampFloat(els.shadowY.value, -40, 40, 0) : 0;
        const shA = shEnabled ? clampFloat(els.shadowAlpha.value, 0, 100, 60) / 100 : 0;
        const shColor = els.shadowColor.value;
        if ((shBlur > 0 || shX !== 0 || shY !== 0) && shA > 0) {
            octx.save();
            octx.shadowColor = hexToRgba(shColor, shA);
            octx.shadowBlur = shBlur;
            octx.shadowOffsetX = shX;
            octx.shadowOffsetY = shY;
            octx.drawImage(layer, 0, 0);
            octx.restore();
        } else {
            octx.drawImage(layer, 0, 0);
        }

        item.outCanvas = out;
        return out;
    }

    // Build a canvas that contains only an outline ring around the silhouette in `stage`,
    // using the shadow trick: render the source many times with a shadow, then cut the source.
    function buildOutlineCanvas(stage, width, color) {
        const pad = Math.ceil(width) + 4;
        const w = stage.width + pad * 2;
        const h = stage.height + pad * 2;
        const c = document.createElement("canvas");
        c.width = w; c.height = h;
        const ctx = c.getContext("2d");
        ctx.shadowColor = color;
        ctx.shadowBlur = width;
        ctx.shadowOffsetX = 0;
        ctx.shadowOffsetY = 0;
        // Multiple passes thicken the shadow into a solid outline.
        for (let i = 0; i < 8; i++) ctx.drawImage(stage, pad, pad);
        ctx.shadowColor = "transparent";
        // Remove the source pixels — keep only the outline ring.
        ctx.globalCompositeOperation = "destination-out";
        ctx.drawImage(stage, pad, pad);
        return c;
    }

    // Box-based dilate (positive r) / erode (negative r) on a mask using max/min filter.
    function morphology(mask, w, h, r) {
        const op = r > 0 ? "max" : "min";
        const radius = Math.abs(r);
        let buf = mask;
        // Horizontal pass
        let tmp = new Uint8ClampedArray(w * h);
        for (let y = 0; y < h; y++) {
            const row = y * w;
            for (let x = 0; x < w; x++) {
                let v = op === "max" ? 0 : 255;
                const x0 = Math.max(0, x - radius);
                const x1 = Math.min(w - 1, x + radius);
                for (let i = x0; i <= x1; i++) {
                    const s = buf[row + i];
                    if (op === "max") { if (s > v) v = s; }
                    else { if (s < v) v = s; }
                }
                tmp[row + x] = v;
            }
        }
        buf = tmp;
        // Vertical pass
        const out = new Uint8ClampedArray(w * h);
        for (let x = 0; x < w; x++) {
            for (let y = 0; y < h; y++) {
                let v = op === "max" ? 0 : 255;
                const y0 = Math.max(0, y - radius);
                const y1 = Math.min(h - 1, y + radius);
                for (let i = y0; i <= y1; i++) {
                    const s = buf[i * w + x];
                    if (op === "max") { if (s > v) v = s; }
                    else { if (s < v) v = s; }
                }
                out[y * w + x] = v;
            }
        }
        return out;
    }

    function hexToRgba(hex, alpha) {
        const [r, g, b] = hexToRgb(hex);
        return `rgba(${r},${g},${b},${alpha})`;
    }

    function drawContain(ctx, src, x, y, w, h) {
        const sw = src.width, sh = src.height;
        const s = Math.min(w / sw, h / sh);
        const dw = sw * s, dh = sh * s;
        ctx.drawImage(src, x + (w - dw) / 2, y + (h - dh) / 2, dw, dh);
    }

    function silhouetteBBox(mask, w, h) {
        let x1 = w, y1 = h, x2 = 0, y2 = 0, found = false;
        for (let y = 0; y < h; y++) {
            for (let x = 0; x < w; x++) {
                if (mask[y * w + x] > 8) {
                    if (x < x1) x1 = x;
                    if (y < y1) y1 = y;
                    if (x > x2) x2 = x;
                    if (y > y2) y2 = y;
                    found = true;
                }
            }
        }
        if (!found) return null;
        return { x1, y1, x2: x2 + 1, y2: y2 + 1 };
    }

    function sampleBgColor(canvas) {
        const mode = els.bgSample.value;
        if (mode === "custom") return hexToRgb(els.bgPick.value);

        const ctx = canvas.getContext("2d");
        const w = canvas.width, h = canvas.height;
        if (mode === "topleft") {
            const d = ctx.getImageData(0, 0, 1, 1).data;
            return [d[0], d[1], d[2]];
        }
        // corners: avg 4 corner samples (4x4 region each)
        const r = 4;
        const samples = [
            ctx.getImageData(0, 0, r, r).data,
            ctx.getImageData(w - r, 0, r, r).data,
            ctx.getImageData(0, h - r, r, r).data,
            ctx.getImageData(w - r, h - r, r, r).data,
        ];
        let sr = 0, sg = 0, sb = 0, n = 0;
        for (const s of samples) {
            for (let i = 0; i < s.length; i += 4) {
                sr += s[i]; sg += s[i + 1]; sb += s[i + 2]; n++;
            }
        }
        return [Math.round(sr / n), Math.round(sg / n), Math.round(sb / n)];
    }

    // ─── Tile UI ─────────────────────────────────────────────────────────
    function renderTile(item) {
        const tile = document.createElement("div");
        tile.className = "preview-tile";
        tile.dataset.id = String(item.id);

        const rm = document.createElement("button");
        rm.className = "remove"; rm.type = "button"; rm.textContent = "×";
        rm.title = "Remove";
        rm.addEventListener("click", () => {
            const idx = items.findIndex((x) => x.id === item.id);
            if (idx >= 0) items.splice(idx, 1);
            tile.remove();
        });
        tile.appendChild(rm);

        const h = document.createElement("h3");
        h.textContent = item.name;
        h.title = item.name;
        tile.appendChild(h);

        const split = document.createElement("div");
        split.className = "split";

        const cellSrc = document.createElement("div");
        cellSrc.className = "cell";
        const labSrc = document.createElement("span");
        labSrc.className = "lab"; labSrc.textContent = "source";
        cellSrc.appendChild(labSrc);
        const srcImg = document.createElement("img");
        srcImg.src = item.srcCanvas.toDataURL();
        cellSrc.appendChild(srcImg);

        const cellOut = document.createElement("div");
        cellOut.className = "cell";
        const labOut = document.createElement("span");
        labOut.className = "lab"; labOut.textContent = "silhouette";
        cellOut.appendChild(labOut);

        split.appendChild(cellSrc);
        split.appendChild(cellOut);
        tile.appendChild(split);

        item.tile = tile;
        item.outCell = cellOut;
        els.previews.appendChild(tile);
    }

    function rebuild(item) {
        try {
            const out = buildSilhouette(item);
            // Replace previous output canvas in the tile.
            const existing = item.outCell.querySelector("canvas");
            if (existing) existing.remove();
            const display = document.createElement("canvas");
            display.width = out.width; display.height = out.height;
            display.getContext("2d").drawImage(out, 0, 0);
            item.outCell.appendChild(display);
        } catch (e) {
            console.error(e);
        }
    }

    function rebuildAll() {
        for (const it of items) rebuild(it);
    }

    // ─── Inputs ──────────────────────────────────────────────────────────
    function setupDropzone() {
        els.dropzone.addEventListener("click", (e) => {
            // Only trigger picker when clicking the empty area, not the URL input/button.
            if (e.target === els.dropzone || e.target.tagName === "P" || e.target.tagName === "B" || e.target.tagName === "KBD") {
                els.filePicker.click();
            }
        });
        els.filePicker.addEventListener("change", async () => {
            const files = Array.from(els.filePicker.files || []);
            await handleFiles(files);
            els.filePicker.value = "";
        });
        ["dragenter", "dragover"].forEach((ev) => {
            els.dropzone.addEventListener(ev, (e) => {
                e.preventDefault(); e.stopPropagation();
                els.dropzone.classList.add("over");
            });
        });
        ["dragleave", "drop"].forEach((ev) => {
            els.dropzone.addEventListener(ev, (e) => {
                e.preventDefault(); e.stopPropagation();
                els.dropzone.classList.remove("over");
            });
        });
        els.dropzone.addEventListener("drop", async (e) => {
            const files = Array.from(e.dataTransfer.files || [])
                .filter((f) => f.type.startsWith("image/") || isZipFile(f));
            if (files.length) await handleFiles(files);
        });

        // Paste from clipboard anywhere on the page.
        window.addEventListener("paste", async (e) => {
            const items = e.clipboardData && e.clipboardData.items;
            if (!items) return;
            const files = [];
            for (const it of items) {
                if (it.kind === "file" && it.type.startsWith("image/")) {
                    const f = it.getAsFile();
                    if (f) files.push(f);
                }
            }
            if (files.length) await handleFiles(files);
        });

        const onUrlAdd = async () => {
            const url = els.urlInput.value.trim();
            if (!url) return;
            try {
                const { img, name, blob } = await loadUrl(url);
                addImage(img, name, blob);
                els.urlInput.value = "";
                setStatus(`Added "${name}"`);
            } catch (e) {
                setStatus("URL error: " + (e.message || e));
            }
        };
        els.urlAdd.addEventListener("click", onUrlAdd);
        els.urlInput.addEventListener("keydown", (e) => {
            if (e.key === "Enter") { e.preventDefault(); onUrlAdd(); }
        });
    }

    async function handleFiles(files) {
        for (const f of files) {
            try {
                if (isZipFile(f)) {
                    await loadBundleZip(f);
                } else {
                    const { img, name, blob } = await loadFile(f);
                    addImage(img, name, blob);
                }
            } catch (e) {
                setStatus(`Failed to load ${f.name}: ${e.message || e}`);
            }
        }
    }

    function isZipFile(f) {
        if (!f) return false;
        if (f.type === "application/zip" || f.type === "application/x-zip-compressed") return true;
        return /\.zip$/i.test(f.name || "");
    }

    function setStatus(s) { els.status.textContent = s || ""; }

    // ─── Range value mirroring ───────────────────────────────────────────
    const rangeMirrors = [
        ["threshold", "thresholdVal", (v) => v],
        ["softness", "softnessVal", (v) => v],
        ["brightness", "brightnessVal", (v) => v],
        ["rotation", "rotationVal", (v) => `${v}°`],
        ["scale", "scaleVal", (v) => `${v}%`],
        ["offsetX", "offsetXVal", (v) => `${v}%`],
        ["offsetY", "offsetYVal", (v) => `${v}%`],
        ["dilate", "dilateVal", (v) => v],
        ["feather", "featherVal", (v) => v],
        ["outlineW", "outlineWVal", (v) => v],
        ["shadowBlur", "shadowBlurVal", (v) => v],
        ["shadowX", "shadowXVal", (v) => v],
        ["shadowY", "shadowYVal", (v) => v],
        ["shadowAlpha", "shadowAlphaVal", (v) => `${v}%`],
    ];

    // Capture initial (default) values of every control so reset works.
    const defaults = {};
    function captureDefault(id) {
        const el = els[id];
        if (!el) return;
        if (el.type === "checkbox") defaults[id] = el.checked;
        else defaults[id] = el.value;
    }

    // Build a compact slider row: [input] [value] [↺] under the label text.
    // If the slider's parent has [data-vertical], stack value+reset below a vertical slider.
    rangeMirrors.forEach(([src, dst, fmt]) => {
        captureDefault(src);
        const input = els[src];
        const valEl = els[dst];
        valEl.className = "slider-val";

        const reset = document.createElement("button");
        reset.type = "button";
        reset.className = "slider-reset";
        reset.textContent = "↺";
        reset.title = `Reset to ${defaults[src]}`;
        reset.addEventListener("click", () => {
            input.value = defaults[src];
            valEl.textContent = fmt(input.value);
            rebuildAll();
        });

        const isVertical = !!input.closest("[data-vertical]");
        if (isVertical) {
            // label > [input, val, reset] — natural column flex from .vslider-grid > label
            input.parentNode.appendChild(valEl);
            input.parentNode.appendChild(reset);
        } else {
            const row = document.createElement("div");
            row.className = "slider-row";
            input.parentNode.insertBefore(row, input);
            row.appendChild(input);
            row.appendChild(valEl);
            row.appendChild(reset);
        }

        const update = () => { valEl.textContent = fmt(input.value); };
        input.addEventListener("input", update);
        update();
    });

    // Re-render on every control change.
    const allControls = [
        "algo", "bgSample", "bgPick",
        "threshold", "softness", "brightness",
        "silColor", "outBg", "outBgTransparent",
        "outWidth", "outHeight", "fitMode", "padPct",
        "fxTransform", "rotation", "scale", "offsetX", "offsetY", "flipH", "flipV",
        "fxMask", "invertMask", "dilate", "feather",
        "fxOutline", "outlineW", "outlineColor",
        "fxShadow", "shadowBlur", "shadowX", "shadowY", "shadowColor", "shadowAlpha",
    ];
    allControls.forEach((id) => {
        captureDefault(id);
        els[id].addEventListener("input", debounce(rebuildAll, 80));
        els[id].addEventListener("change", debounce(rebuildAll, 80));
    });

    // Card enable/disable visual sync.
    const fxCards = [
        ["fxTransform", "transform"],
        ["fxMask", "mask"],
        ["fxOutline", "outline"],
        ["fxShadow", "shadow"],
    ];
    function syncCards() {
        for (const [id, name] of fxCards) {
            const card = document.querySelector(`.effect-card[data-fx="${name}"]`);
            if (!card) continue;
            card.classList.toggle("disabled", !els[id].checked);
        }
    }
    fxCards.forEach(([id]) => els[id].addEventListener("change", syncCards));
    syncCards();

    function resetIds(ids) {
        ids.forEach((id) => {
            const el = els[id];
            if (!el || !(id in defaults)) return;
            if (el.type === "checkbox") el.checked = defaults[id];
            else el.value = defaults[id];
        });
        rangeMirrors.forEach(([src, dst, fmt]) => {
            if (ids.includes(src)) els[dst].textContent = fmt(els[src].value);
        });
        syncCards();
        rebuildAll();
    }

    els.resetTransform.addEventListener("click", (e) => {
        e.preventDefault();
        resetIds(["fxTransform", "rotation", "scale", "offsetX", "offsetY", "flipH", "flipV"]);
    });
    els.resetMask.addEventListener("click", (e) => {
        e.preventDefault();
        resetIds(["fxMask", "invertMask", "dilate", "feather"]);
    });
    els.resetOutline.addEventListener("click", (e) => {
        e.preventDefault();
        resetIds(["fxOutline", "outlineW", "outlineColor"]);
    });
    els.resetShadow.addEventListener("click", (e) => {
        e.preventDefault();
        resetIds(["fxShadow", "shadowBlur", "shadowX", "shadowY", "shadowColor", "shadowAlpha"]);
    });
    els.resetAll.addEventListener("click", () => {
        resetIds(allControls);
    });

    // Preview background picker.
    function applyPreviewBg() {
        if (els.pageBgCheck.checked) {
            els.previews.classList.remove("solid-bg");
            els.previews.style.removeProperty("--preview-bg");
        } else {
            els.previews.classList.add("solid-bg");
            els.previews.style.setProperty("--preview-bg", els.pageBg.value);
        }
    }
    els.pageBg.addEventListener("input", applyPreviewBg);
    els.pageBgCheck.addEventListener("change", applyPreviewBg);
    applyPreviewBg();

    function debounce(fn, ms) {
        let t; return function () { clearTimeout(t); t = setTimeout(fn, ms); };
    }

    // ─── Download ────────────────────────────────────────────────────────
    function canvasToBlob(canvas, format, bg) {
        return new Promise((resolve, reject) => {
            const mime = format === "jpg" ? "image/jpeg" : "image/png";
            if (mime === "image/jpeg" && bg) {
                // Composite onto solid bg first so JPG isn't black.
                const c = document.createElement("canvas");
                c.width = canvas.width; c.height = canvas.height;
                const ctx = c.getContext("2d");
                ctx.fillStyle = bg; ctx.fillRect(0, 0, c.width, c.height);
                ctx.drawImage(canvas, 0, 0);
                c.toBlob((b) => b ? resolve(b) : reject(new Error("toBlob failed")), mime, 0.95);
            } else {
                canvas.toBlob((b) => b ? resolve(b) : reject(new Error("toBlob failed")), mime,
                    mime === "image/jpeg" ? 0.95 : undefined);
            }
        });
    }

    function downloadBlob(blob, filename) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url; a.download = filename;
        document.body.appendChild(a); a.click(); a.remove();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    }

    async function onDownloadAll() {
        if (!items.length) { setStatus("Nothing to download."); return; }
        els.downloadAll.disabled = true;
        setStatus("Rendering…");
        try {
            const format = els.format.value;
            const suffix = els.suffix.value.trim();
            const jpgBg = format === "jpg"
                ? (els.outBgTransparent.checked ? "#000000" : els.outBg.value)
                : null;

            const out = [];
            for (const it of items) {
                if (!it.outCanvas) buildSilhouette(it);
                const blob = await canvasToBlob(it.outCanvas, format, jpgBg);
                const name = sanitizeFileName(baseName(it.name)) + suffix + "." + format;
                out.push({ name, blob, item: it });
            }

            const zip = new JSZip();
            const imgsDir = zip.folder("images");
            const origDir = zip.folder("originals");
            for (const f of out) {
                imgsDir.file(f.name, f.blob);
                // Original: prefer stored blob; fall back to re-encoding srcCanvas as PNG.
                const origName = sanitizeFileName(f.item.name) || (sanitizeFileName(baseName(f.item.name)) + ".png");
                if (f.item.originalBlob) {
                    origDir.file(origName, f.item.originalBlob);
                } else {
                    const pngBlob = await new Promise((res) =>
                        f.item.srcCanvas.toBlob((b) => res(b), "image/png"));
                    if (pngBlob) {
                        const fallback = sanitizeFileName(baseName(f.item.name)) + ".png";
                        origDir.file(fallback, pngBlob);
                    }
                }
            }
            // Always bundle the current settings as a preset JSON next to the images.
            const presetJson = JSON.stringify(serializeSettings(), null, 2);
            zip.file("preset.json", presetJson);

            const zipBlob = await zip.generateAsync({ type: "blob" });
            const stamp = new Date().toISOString().replace(/[:.]/g, "-").slice(0, 19);
            downloadBlob(zipBlob, `silhouettes_${stamp}.zip`);

            setStatus(`Exported ${out.length} image(s) + originals + preset.json`);
        } catch (e) {
            setStatus("Error: " + (e.message || e));
        } finally {
            els.downloadAll.disabled = false;
        }
    }

    els.downloadAll.addEventListener("click", onDownloadAll);
    els.clearAll.addEventListener("click", () => {
        items.length = 0;
        els.previews.innerHTML = "";
        setStatus("");
    });

    // ─── Presets (save/load all settings) ────────────────────────────────
    const presetControls = allControls.concat(["suffix", "format", "pageBg", "pageBgCheck"]);

    function serializeSettings() {
        const obj = { _tool: "silhouette-maker", _version: 1 };
        for (const id of presetControls) {
            const el = els[id];
            if (!el) continue;
            obj[id] = (el.type === "checkbox") ? el.checked : el.value;
        }
        return obj;
    }

    function applySettings(obj) {
        if (!obj || typeof obj !== "object") throw new Error("Not a JSON object");
        let applied = 0;
        for (const id of presetControls) {
            if (!(id in obj)) continue;
            const el = els[id];
            if (!el) continue;
            if (el.type === "checkbox") el.checked = !!obj[id];
            else el.value = String(obj[id]);
            applied++;
        }
        // Refresh value mirrors and any visual sync.
        rangeMirrors.forEach(([src, dst, fmt]) => {
            els[dst].textContent = fmt(els[src].value);
        });
        syncCards();
        applyPreviewBg();
        rebuildAll();
        return applied;
    }

    function setPresetStatus(s) { els.presetStatus.textContent = s || ""; }

    els.presetSave.addEventListener("click", () => {
        const obj = serializeSettings();
        const json = JSON.stringify(obj, null, 2);
        const blob = new Blob([json], { type: "application/json" });
        const stamp = new Date().toISOString().replace(/[:.]/g, "-").slice(0, 19);
        downloadBlob(blob, `silhouette-preset_${stamp}.json`);
        setPresetStatus("Preset downloaded.");
    });

    els.presetLoadBtn.addEventListener("click", () => els.presetFile.click());
    els.presetFile.addEventListener("change", async () => {
        const f = els.presetFile.files && els.presetFile.files[0];
        els.presetFile.value = "";
        if (!f) return;
        try {
            if (isZipFile(f)) {
                await loadBundleZip(f);
                setPresetStatus(`Loaded bundle "${f.name}".`);
                return;
            }
            const text = await f.text();
            const obj = JSON.parse(text);
            const n = applySettings(obj);
            els.presetText.value = JSON.stringify(obj, null, 2);
            setPresetStatus(`Applied ${n} settings from "${f.name}".`);
        } catch (e) {
            setPresetStatus("Load error: " + (e.message || e));
        }
    });

    els.presetCopy.addEventListener("click", async () => {
        const obj = serializeSettings();
        const json = JSON.stringify(obj, null, 2);
        els.presetText.value = json;
        try {
            await navigator.clipboard.writeText(json);
            setPresetStatus("Copied to clipboard & JSON box.");
        } catch {
            setPresetStatus("Copied to JSON box (clipboard blocked).");
        }
    });

    els.presetApply.addEventListener("click", () => {
        const text = els.presetText.value.trim();
        if (!text) { setPresetStatus("JSON box is empty."); return; }
        try {
            const obj = JSON.parse(text);
            const n = applySettings(obj);
            setPresetStatus(`Applied ${n} settings from JSON box.`);
        } catch (e) {
            setPresetStatus("Parse error: " + (e.message || e));
        }
    });

    setupDropzone();
})();
