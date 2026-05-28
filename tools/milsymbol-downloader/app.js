/* milsymbol downloader - simple SIDC -> SVG/PNG/JPG exporter */
(function () {
    "use strict";

    const $ = (id) => document.getElementById(id);

    const els = {
        codes: $("codes"),
        format: $("format"),
        width: $("width"),
        height: $("height"),
        symbolSize: $("symbolSize"),
        padding: $("padding"),
        standard: $("standard"),
        monoColor: $("monoColor"),
        fitMode: $("fitMode"),
        preview: $("preview"),
        downloadAll: $("downloadAll"),
        status: $("status"),
        loadSamples: $("loadSamples"),
        clearCodes: $("clearCodes"),
    };

    const SAMPLES = [
        "SFGPUCI-----E", // friendly infantry
        "SHGPUCI-----E", // hostile infantry
        "SNGPUCI-----E", // neutral infantry
        "SUGPUCI-----E", // unknown infantry
        "SFGPUCA-----E", // friendly armor
        "SFGPUCRV----E", // friendly recon
        "SFGPUCF-----E", // friendly field artillery
        "SFGPUCE-----E", // friendly engineer
        "SFGPUH------E", // friendly headquarters
        "SFAPMFF-----E", // friendly fighter aircraft
        "SFSPCLBB----E", // friendly battleship
        "SFGPEWRR----E", // friendly radar
    ];

    function getOptions() {
        const opts = {
            size: clampInt(els.symbolSize.value, 8, 2048, 100),
            standard: els.standard.value,
        };
        if (els.monoColor.value) {
            opts.monoColor = els.monoColor.value;
            opts.fill = false;
        }
        return opts;
    }

    function clampInt(v, lo, hi, def) {
        const n = parseInt(v, 10);
        if (!Number.isFinite(n)) return def;
        return Math.min(hi, Math.max(lo, n));
    }

    function buildSymbol(sidc) {
        // milsymbol exposes ms.Symbol
        return new ms.Symbol(sidc, getOptions());
    }

    /**
     * Render an SVG string into a target-sized SVG (with viewBox math so it
     * fits inside width x height with padding).
     */
    function fitSvg(rawSvg, targetW, targetH, padding, fitMode) {
        const parser = new DOMParser();
        const doc = parser.parseFromString(rawSvg, "image/svg+xml");
        const src = doc.documentElement;

        // milsymbol gives us width/height + viewBox; normalize.
        let vb = src.getAttribute("viewBox");
        if (!vb) {
            const w = parseFloat(src.getAttribute("width")) || 100;
            const h = parseFloat(src.getAttribute("height")) || 100;
            vb = `0 0 ${w} ${h}`;
            src.setAttribute("viewBox", vb);
        }
        const [vx, vy, vw, vh] = vb.split(/\s+/).map(parseFloat);

        const innerW = Math.max(1, targetW - padding * 2);
        const innerH = Math.max(1, targetH - padding * 2);

        let scale, drawW, drawH;
        if (fitMode === "stretch") {
            drawW = innerW; drawH = innerH;
        } else {
            scale = Math.min(innerW / vw, innerH / vh);
            drawW = vw * scale;
            drawH = vh * scale;
        }
        const offX = (targetW - drawW) / 2;
        const offY = (targetH - drawH) / 2;

        // Build a new SVG that wraps the original content.
        const NS = "http://www.w3.org/2000/svg";
        const out = document.createElementNS(NS, "svg");
        out.setAttribute("xmlns", NS);
        out.setAttribute("width", String(targetW));
        out.setAttribute("height", String(targetH));
        out.setAttribute("viewBox", `0 0 ${targetW} ${targetH}`);

        const g = document.createElementNS(NS, "g");
        if (fitMode === "stretch") {
            g.setAttribute(
                "transform",
                `translate(${offX} ${offY}) scale(${drawW / vw} ${drawH / vh}) translate(${-vx} ${-vy})`
            );
        } else {
            g.setAttribute(
                "transform",
                `translate(${offX} ${offY}) scale(${scale}) translate(${-vx} ${-vy})`
            );
        }
        // Move all children of source <svg> into the group.
        while (src.firstChild) g.appendChild(src.firstChild);
        out.appendChild(g);

        return new XMLSerializer().serializeToString(out);
    }

    function svgToBlob(svgString) {
        return new Blob([svgString], { type: "image/svg+xml;charset=utf-8" });
    }

    function svgToRaster(svgString, w, h, mime, background) {
        return new Promise((resolve, reject) => {
            const blob = svgToBlob(svgString);
            const url = URL.createObjectURL(blob);
            const img = new Image();
            img.onload = () => {
                const canvas = document.createElement("canvas");
                canvas.width = w;
                canvas.height = h;
                const ctx = canvas.getContext("2d");
                if (background) {
                    ctx.fillStyle = background;
                    ctx.fillRect(0, 0, w, h);
                }
                ctx.drawImage(img, 0, 0, w, h);
                URL.revokeObjectURL(url);
                canvas.toBlob(
                    (b) => (b ? resolve(b) : reject(new Error("toBlob failed"))),
                    mime,
                    mime === "image/jpeg" ? 0.95 : undefined
                );
            };
            img.onerror = (e) => {
                URL.revokeObjectURL(url);
                reject(new Error("SVG rasterize failed"));
            };
            img.src = url;
        });
    }

    function sanitizeFileName(name) {
        return name.replace(/[^A-Za-z0-9._-]/g, "_");
    }

    function getCodes() {
        return els.codes.value
            .split(/\r?\n/)
            .map((s) => s.trim())
            .filter(Boolean);
    }

    async function renderOne(sidc) {
        const w = clampInt(els.width.value, 8, 4096, 256);
        const h = clampInt(els.height.value, 8, 4096, 256);
        const padding = clampInt(els.padding.value, 0, 512, 8);
        const fitMode = els.fitMode.value;
        const format = els.format.value;

        const sym = buildSymbol(sidc);
        const rawSvg = sym.asSVG();
        const finalSvg = fitSvg(rawSvg, w, h, padding, fitMode);

        let blob, ext, mime;
        if (format === "svg") {
            blob = svgToBlob(finalSvg);
            ext = "svg";
            mime = "image/svg+xml";
        } else if (format === "png") {
            blob = await svgToRaster(finalSvg, w, h, "image/png", null);
            ext = "png";
            mime = "image/png";
        } else {
            blob = await svgToRaster(finalSvg, w, h, "image/jpeg", "#ffffff");
            ext = "jpg";
            mime = "image/jpeg";
        }
        return { sidc, blob, ext, mime, svg: finalSvg };
    }

    async function updatePreview() {
        const codes = getCodes();
        els.preview.innerHTML = "";
        if (!codes.length) {
            els.preview.innerHTML = '<p class="hint">Enter at least one SIDC above.</p>';
            return;
        }
        const w = clampInt(els.width.value, 8, 4096, 256);
        const h = clampInt(els.height.value, 8, 4096, 256);
        const padding = clampInt(els.padding.value, 0, 512, 8);
        const fitMode = els.fitMode.value;

        for (const sidc of codes) {
            const tile = document.createElement("div");
            tile.className = "tile";
            const wrap = document.createElement("div");
            wrap.className = "img-wrap";
            const code = document.createElement("div");
            code.className = "code";
            code.textContent = sidc;
            tile.appendChild(wrap);
            tile.appendChild(code);
            els.preview.appendChild(tile);

            try {
                const sym = buildSymbol(sidc);
                const svgStr = fitSvg(sym.asSVG(), w, h, padding, fitMode);
                // Display preview inline via blob URL so we see exactly what gets exported.
                const url = URL.createObjectURL(svgToBlob(svgStr));
                const img = new Image();
                img.src = url;
                img.alt = sidc;
                img.onload = () => URL.revokeObjectURL(url);
                wrap.appendChild(img);
            } catch (e) {
                tile.classList.add("error");
                wrap.textContent = "✕";
                code.textContent = sidc + " — " + (e && e.message ? e.message : "error");
            }
        }
    }

    function downloadBlob(blob, filename) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        a.remove();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    }

    async function onDownloadAll() {
        const codes = getCodes();
        if (!codes.length) {
            els.status.textContent = "No codes to export.";
            return;
        }
        els.downloadAll.disabled = true;
        els.status.textContent = "Rendering…";
        try {
            const results = [];
            const errors = [];
            for (const sidc of codes) {
                try {
                    results.push(await renderOne(sidc));
                } catch (e) {
                    errors.push(sidc + ": " + (e && e.message ? e.message : "error"));
                }
            }

            if (results.length === 1) {
                const r = results[0];
                downloadBlob(r.blob, sanitizeFileName(r.sidc) + "." + r.ext);
            } else if (results.length > 1) {
                const zip = new JSZip();
                for (const r of results) {
                    zip.file(sanitizeFileName(r.sidc) + "." + r.ext, r.blob);
                }
                const zipBlob = await zip.generateAsync({ type: "blob" });
                const stamp = new Date().toISOString().replace(/[:.]/g, "-").slice(0, 19);
                downloadBlob(zipBlob, `milsymbols_${stamp}.zip`);
            }

            const msg = [];
            if (results.length) msg.push(`Exported ${results.length}`);
            if (errors.length) msg.push(`${errors.length} failed`);
            els.status.textContent = msg.join(", ") + (errors.length ? " — " + errors.join("; ") : "");
        } finally {
            els.downloadAll.disabled = false;
        }
    }

    // Wire up events
    const previewDebounced = debounce(updatePreview, 150);
    [
        "codes", "format", "width", "height",
        "symbolSize", "padding", "standard", "monoColor", "fitMode",
    ].forEach((id) => {
        els[id].addEventListener("input", previewDebounced);
        els[id].addEventListener("change", previewDebounced);
    });

    els.downloadAll.addEventListener("click", onDownloadAll);
    els.loadSamples.addEventListener("click", () => {
        els.codes.value = SAMPLES.join("\n");
        updatePreview();
    });
    els.clearCodes.addEventListener("click", () => {
        els.codes.value = "";
        updatePreview();
    });

    function debounce(fn, ms) {
        let t;
        return function () {
            clearTimeout(t);
            t = setTimeout(fn, ms);
        };
    }

    // Initial render
    if (typeof ms === "undefined") {
        els.preview.innerHTML =
            '<p class="hint" style="color:var(--danger)">milsymbol failed to load. Check your internet connection (CDN).</p>';
    } else {
        updatePreview();
    }
})();
