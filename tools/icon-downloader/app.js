/* icon downloader -- Iconify-powered icon search + batch SVG/PNG/JPG export */
(function () {
    "use strict";

    const API = "https://api.iconify.design";
    const $ = (id) => document.getElementById(id);

    const els = {
        query: $("query"),
        collection: $("collection"),
        searchBtn: $("searchBtn"),
        results: $("results"),
        loadMore: $("loadMore"),
        searchStatus: $("searchStatus"),
        format: $("format"),
        width: $("width"),
        height: $("height"),
        padding: $("padding"),
        iconColor: $("iconColor"),
        iconKeepColor: $("iconKeepColor"),
        bgColor: $("bgColor"),
        bgTransparent: $("bgTransparent"),
        fitMode: $("fitMode"),
        suffix: $("suffix"),
        selected: $("selected"),
        downloadAll: $("downloadAll"),
        clearSelected: $("clearSelected"),
        status: $("status"),
    };

    // ─── License info shown in attribution file ──────────────────────────
    const LICENSES = {
        "game-icons": "CC-BY 3.0 — https://game-icons.net (Lorc, Delapouite, contributors)",
        "lucide": "ISC — https://lucide.dev",
        "ph": "MIT — https://phosphoricons.com",
        "mdi": "Apache 2.0 — https://pictogrammers.com/library/mdi/",
        "tabler": "MIT — https://tabler.io/icons",
        "fa6-solid": "CC-BY 4.0 — https://fontawesome.com",
        "material-symbols": "Apache 2.0 — https://fonts.google.com/icons",
    };
    const REQUIRES_ATTRIBUTION = new Set(["game-icons", "fa6-solid"]);

    // ─── State ───────────────────────────────────────────────────────────
    const selected = new Map(); // key "prefix:name" -> { prefix, name }
    let searchOffset = 0;
    let lastQuery = "";
    let lastCollection = "";

    function clampInt(v, lo, hi, def) {
        const n = parseInt(v, 10);
        if (!Number.isFinite(n)) return def;
        return Math.min(hi, Math.max(lo, n));
    }

    function sanitizeFileName(name) {
        return name.replace(/[^A-Za-z0-9._-]/g, "_");
    }

    // ─── Iconify API ─────────────────────────────────────────────────────
    async function searchIcons(query, collection, limit, start) {
        const params = new URLSearchParams({
            query: query || "",
            limit: String(limit),
            start: String(start || 0),
        });
        if (collection) params.set("prefix", collection);
        const r = await fetch(`${API}/search?${params}`);
        if (!r.ok) throw new Error("Search failed: HTTP " + r.status);
        return r.json();
    }

    /**
     * Fetch raw SVG for a single icon. Returns SVG string.
     * When color !== null we pass it as the `color` query parameter, which
     * Iconify uses for monochrome icons (all currentColor strokes/fills).
     */
    async function fetchIconSvg(prefix, name, color) {
        const params = new URLSearchParams();
        if (color) params.set("color", color);
        const qs = params.toString();
        const url = `${API}/${prefix}/${name}.svg${qs ? "?" + qs : ""}`;
        const r = await fetch(url);
        if (!r.ok) throw new Error(`Fetch ${prefix}:${name} failed`);
        return r.text();
    }

    // ─── SVG fit / wrap ──────────────────────────────────────────────────
    function fitSvg(rawSvg, targetW, targetH, padding, fitMode, bgColor) {
        const doc = new DOMParser().parseFromString(rawSvg, "image/svg+xml");
        const src = doc.documentElement;

        let vb = src.getAttribute("viewBox");
        if (!vb) {
            const w = parseFloat(src.getAttribute("width")) || 24;
            const h = parseFloat(src.getAttribute("height")) || 24;
            vb = `0 0 ${w} ${h}`;
        }
        const [vx, vy, vw, vh] = vb.split(/\s+/).map(parseFloat);

        const innerW = Math.max(1, targetW - padding * 2);
        const innerH = Math.max(1, targetH - padding * 2);

        let drawW, drawH, scale;
        if (fitMode === "stretch") {
            drawW = innerW; drawH = innerH;
        } else {
            scale = Math.min(innerW / vw, innerH / vh);
            drawW = vw * scale;
            drawH = vh * scale;
        }
        const offX = (targetW - drawW) / 2;
        const offY = (targetH - drawH) / 2;

        const NS = "http://www.w3.org/2000/svg";
        const out = document.createElementNS(NS, "svg");
        out.setAttribute("xmlns", NS);
        out.setAttribute("width", String(targetW));
        out.setAttribute("height", String(targetH));
        out.setAttribute("viewBox", `0 0 ${targetW} ${targetH}`);
        out.setAttribute("overflow", "hidden");

        if (bgColor) {
            const bg = document.createElementNS(NS, "rect");
            bg.setAttribute("x", "0"); bg.setAttribute("y", "0");
            bg.setAttribute("width", String(targetW));
            bg.setAttribute("height", String(targetH));
            bg.setAttribute("fill", bgColor);
            out.appendChild(bg);
        }

        const g = document.createElementNS(NS, "g");
        if (fitMode === "stretch") {
            g.setAttribute("transform",
                `translate(${offX} ${offY}) scale(${drawW / vw} ${drawH / vh}) translate(${-vx} ${-vy})`);
        } else {
            g.setAttribute("transform",
                `translate(${offX} ${offY}) scale(${scale}) translate(${-vx} ${-vy})`);
        }
        while (src.firstChild) g.appendChild(src.firstChild);
        out.appendChild(g);

        return new XMLSerializer().serializeToString(out);
    }

    function svgToBlob(svg) {
        return new Blob([svg], { type: "image/svg+xml;charset=utf-8" });
    }

    function svgToRaster(svg, w, h, mime, background) {
        return new Promise((resolve, reject) => {
            const url = URL.createObjectURL(svgToBlob(svg));
            const img = new Image();
            img.onload = () => {
                const canvas = document.createElement("canvas");
                canvas.width = w; canvas.height = h;
                const ctx = canvas.getContext("2d");
                if (background) { ctx.fillStyle = background; ctx.fillRect(0, 0, w, h); }
                ctx.drawImage(img, 0, 0, w, h);
                URL.revokeObjectURL(url);
                canvas.toBlob(
                    (b) => (b ? resolve(b) : reject(new Error("toBlob failed"))),
                    mime,
                    mime === "image/jpeg" ? 0.95 : undefined
                );
            };
            img.onerror = () => { URL.revokeObjectURL(url); reject(new Error("rasterize failed")); };
            img.src = url;
        });
    }

    // ─── UI helpers ──────────────────────────────────────────────────────
    function currentColor() {
        return els.iconKeepColor.checked ? null : els.iconColor.value;
    }
    function currentBg() {
        return els.bgTransparent.checked ? "" : els.bgColor.value;
    }

    function makeResultTile(prefix, name) {
        const key = `${prefix}:${name}`;
        const tile = document.createElement("div");
        tile.className = "icon-tile";
        tile.dataset.key = key;
        if (selected.has(key)) tile.classList.add("selected");

        const coll = document.createElement("span");
        coll.className = "collection";
        coll.textContent = prefix;
        coll.title = LICENSES[prefix] || prefix;
        tile.appendChild(coll);

        const wrap = document.createElement("div");
        wrap.className = "img-wrap";
        const img = document.createElement("img");
        // Preview color = user choice (or original)
        const color = currentColor();
        const params = new URLSearchParams();
        if (color) params.set("color", color);
        params.set("height", "64");
        img.src = `${API}/${prefix}/${name}.svg?${params}`;
        img.alt = name;
        img.loading = "lazy";
        wrap.appendChild(img);
        tile.appendChild(wrap);

        const lab = document.createElement("div");
        lab.className = "label";
        lab.textContent = name;
        lab.title = `${prefix}:${name}`;
        tile.appendChild(lab);

        tile.addEventListener("click", () => toggleSelected(prefix, name));
        return tile;
    }

    function toggleSelected(prefix, name) {
        const key = `${prefix}:${name}`;
        if (selected.has(key)) selected.delete(key);
        else selected.set(key, { prefix, name });
        renderSelected();
        // Refresh tile state in results
        const t = els.results.querySelector(`[data-key="${cssEscape(key)}"]`);
        if (t) t.classList.toggle("selected", selected.has(key));
    }

    function cssEscape(s) {
        return s.replace(/["\\]/g, "\\$&");
    }

    function renderSelected() {
        els.selected.innerHTML = "";
        if (!selected.size) {
            els.selected.innerHTML = '<p class="hint" style="grid-column:1/-1;margin:0">Click icons in the search results to add them here.</p>';
            return;
        }
        const color = currentColor();
        for (const { prefix, name } of selected.values()) {
            const tile = document.createElement("div");
            tile.className = "icon-tile selected";

            const coll = document.createElement("span");
            coll.className = "collection";
            coll.textContent = prefix;
            coll.title = LICENSES[prefix] || prefix;
            tile.appendChild(coll);

            const rm = document.createElement("button");
            rm.className = "remove";
            rm.type = "button";
            rm.textContent = "×";
            rm.title = "Remove";
            rm.addEventListener("click", (e) => {
                e.stopPropagation();
                selected.delete(`${prefix}:${name}`);
                renderSelected();
                const t = els.results.querySelector(`[data-key="${cssEscape(prefix + ":" + name)}"]`);
                if (t) t.classList.remove("selected");
            });
            tile.appendChild(rm);

            const wrap = document.createElement("div");
            wrap.className = "img-wrap";
            const img = document.createElement("img");
            const params = new URLSearchParams();
            if (color) params.set("color", color);
            params.set("height", "64");
            img.src = `${API}/${prefix}/${name}.svg?${params}`;
            img.alt = name;
            wrap.appendChild(img);
            tile.appendChild(wrap);

            const lab = document.createElement("div");
            lab.className = "label";
            lab.textContent = name;
            lab.title = `${prefix}:${name}`;
            tile.appendChild(lab);
            els.selected.appendChild(tile);
        }
    }

    // ─── Search flow ─────────────────────────────────────────────────────
    async function runSearch(reset) {
        if (reset) {
            searchOffset = 0;
            els.results.innerHTML = "";
            lastQuery = els.query.value.trim();
            lastCollection = els.collection.value;
        }
        const q = lastQuery;
        if (!q) {
            els.searchStatus.textContent = "Type something to search.";
            return;
        }
        els.searchBtn.disabled = true;
        els.searchStatus.textContent = "Searching…";
        try {
            const data = await searchIcons(q, lastCollection, 64, searchOffset);
            const icons = data.icons || [];
            for (const id of icons) {
                const [prefix, name] = id.split(":");
                if (prefix && name) els.results.appendChild(makeResultTile(prefix, name));
            }
            searchOffset += icons.length;
            const total = data.total || icons.length;
            els.searchStatus.textContent = `Showing ${els.results.childElementCount} of ${total}`;
            els.loadMore.hidden = els.results.childElementCount >= total;
            if (!icons.length && searchOffset === 0) {
                els.results.innerHTML = '<p class="hint" style="grid-column:1/-1">No icons matched.</p>';
            }
        } catch (e) {
            els.searchStatus.textContent = "Error: " + (e.message || e);
        } finally {
            els.searchBtn.disabled = false;
        }
    }

    // ─── Download flow ───────────────────────────────────────────────────
    async function renderOne(prefix, name) {
        const w = clampInt(els.width.value, 8, 4096, 256);
        const h = clampInt(els.height.value, 8, 4096, 256);
        const padding = clampInt(els.padding.value, 0, 512, 0);
        const fitMode = els.fitMode.value;
        const format = els.format.value;
        const color = currentColor();
        const bg = currentBg();

        const rawSvg = await fetchIconSvg(prefix, name, color);
        const finalSvg = fitSvg(rawSvg, w, h, padding, fitMode, bg);

        let blob, ext;
        if (format === "svg") { blob = svgToBlob(finalSvg); ext = "svg"; }
        else if (format === "png") { blob = await svgToRaster(finalSvg, w, h, "image/png", null); ext = "png"; }
        else { blob = await svgToRaster(finalSvg, w, h, "image/jpeg", bg || "#ffffff"); ext = "jpg"; }

        return { prefix, name, blob, ext };
    }

    function downloadBlob(blob, filename) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url; a.download = filename;
        document.body.appendChild(a); a.click(); a.remove();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    }

    function buildAttribution(items) {
        const usedPrefixes = new Set(items.map((i) => i.prefix));
        const lines = [
            "Icons exported via the Iconify API (https://iconify.design).",
            "",
            "Collections used and their licenses:",
        ];
        for (const p of usedPrefixes) {
            lines.push(`- ${p}: ${LICENSES[p] || "see icon set homepage"}`);
        }
        const needsAttr = [...usedPrefixes].filter((p) => REQUIRES_ATTRIBUTION.has(p));
        if (needsAttr.length) {
            lines.push("",
                "The following collections REQUIRE attribution when used (BY clauses):",
                ...needsAttr.map((p) => `- ${p}`),
                "Include this notice in your product credits."
            );
        }
        lines.push("", "Icons included:");
        for (const i of items) lines.push(`- ${i.prefix}:${i.name}`);
        return lines.join("\n");
    }

    async function onDownloadAll() {
        if (!selected.size) { els.status.textContent = "Nothing selected."; return; }
        const suffix = els.suffix.value.trim();
        els.downloadAll.disabled = true;
        els.status.textContent = "Rendering…";
        try {
            const results = [];
            const errors = [];
            for (const { prefix, name } of selected.values()) {
                try { results.push(await renderOne(prefix, name)); }
                catch (e) { errors.push(`${prefix}:${name}: ${e.message || e}`); }
            }

            const filenameFor = (r) =>
                sanitizeFileName(`${r.prefix}_${r.name}${suffix}`) + "." + r.ext;

            if (results.length === 1 && !needsAttributionFor(results)) {
                downloadBlob(results[0].blob, filenameFor(results[0]));
            } else if (results.length >= 1) {
                const zip = new JSZip();
                for (const r of results) zip.file(filenameFor(r), r.blob);
                if (needsAttributionFor(results)) {
                    zip.file("ATTRIBUTION.txt", buildAttribution(results));
                }
                const zipBlob = await zip.generateAsync({ type: "blob" });
                const stamp = new Date().toISOString().replace(/[:.]/g, "-").slice(0, 19);
                downloadBlob(zipBlob, `icons_${stamp}.zip`);
            }

            const msg = [];
            if (results.length) msg.push(`Exported ${results.length}`);
            if (errors.length) msg.push(`${errors.length} failed`);
            els.status.textContent = msg.join(", ") + (errors.length ? " — " + errors.join("; ") : "");
        } finally {
            els.downloadAll.disabled = false;
        }
    }

    function needsAttributionFor(items) {
        return items.some((i) => REQUIRES_ATTRIBUTION.has(i.prefix));
    }

    // ─── Wire up ─────────────────────────────────────────────────────────
    els.searchBtn.addEventListener("click", () => runSearch(true));
    els.query.addEventListener("keydown", (e) => {
        if (e.key === "Enter") { e.preventDefault(); runSearch(true); }
    });
    els.collection.addEventListener("change", () => runSearch(true));
    els.loadMore.addEventListener("click", () => runSearch(false));
    els.downloadAll.addEventListener("click", onDownloadAll);
    els.clearSelected.addEventListener("click", () => {
        selected.clear();
        renderSelected();
        els.results.querySelectorAll(".icon-tile.selected").forEach((t) => t.classList.remove("selected"));
    });

    // Re-render previews when color settings change.
    ["iconColor", "iconKeepColor"].forEach((id) => {
        els[id].addEventListener("input", () => {
            // Refresh src on every tile (cheap, browser will cache).
            const refresh = (parent) => {
                parent.querySelectorAll(".icon-tile").forEach((tile) => {
                    const img = tile.querySelector("img");
                    if (!img) return;
                    const url = new URL(img.src);
                    url.searchParams.delete("color");
                    const color = currentColor();
                    if (color) url.searchParams.set("color", color);
                    img.src = url.toString();
                });
            };
            refresh(els.results);
            refresh(els.selected);
        });
    });

    // Initial helpful default search so the page isn't empty.
    renderSelected();
    els.query.value = "sword";
    runSearch(true);
})();
