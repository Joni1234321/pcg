/* silhouette-finder -- Wikimedia Commons-backed search + recolor + batch export */
(function () {
    "use strict";

    const COMMONS_API = "https://commons.wikimedia.org/w/api.php";
    const $ = (id) => document.getElementById(id);

    const els = {
        query: $("query"),
        preset: $("preset"),
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

    // selected: map title -> { title, thumbUrl, fileUrl, license, author, attribution, descriptionUrl }
    const selected = new Map();
    let searchOffset = 0;
    let lastFullQuery = "";

    // Cache fetched/recolored SVG strings to avoid re-downloading on every preview tweak.
    const svgCache = new Map(); // title -> raw SVG string

    function clampInt(v, lo, hi, def) {
        const n = parseInt(v, 10);
        if (!Number.isFinite(n)) return def;
        return Math.min(hi, Math.max(lo, n));
    }
    function sanitizeFileName(name) {
        return name.replace(/^File:/i, "").replace(/[^A-Za-z0-9._-]/g, "_");
    }
    function htmlToText(html) {
        const d = document.createElement("div");
        d.innerHTML = html || "";
        return (d.textContent || "").trim();
    }
    function currentColor() { return els.iconKeepColor.checked ? null : els.iconColor.value; }
    function currentBg() { return els.bgTransparent.checked ? "" : els.bgColor.value; }

    // ─── Commons API ─────────────────────────────────────────────────────
    async function searchCommons(query, limit, offset) {
        // generator=search restricted to file namespace, only return SVGs via prop=imageinfo
        const params = new URLSearchParams({
            action: "query",
            format: "json",
            origin: "*",
            generator: "search",
            gsrnamespace: "6",
            gsrsearch: `${query} filetype:svg`,
            gsrlimit: String(limit),
            gsroffset: String(offset || 0),
            prop: "imageinfo",
            iiprop: "url|extmetadata|mime|size",
            iiurlwidth: "240",
        });
        const r = await fetch(`${COMMONS_API}?${params}`);
        if (!r.ok) throw new Error("Search failed: HTTP " + r.status);
        const data = await r.json();
        const pages = (data && data.query && data.query.pages) || {};
        const results = [];
        for (const id of Object.keys(pages)) {
            const p = pages[id];
            const ii = p.imageinfo && p.imageinfo[0];
            if (!ii) continue;
            if (ii.mime !== "image/svg+xml") continue;
            const meta = ii.extmetadata || {};
            results.push({
                title: p.title,
                pageId: p.pageid,
                index: p.index || 0,
                thumbUrl: ii.thumburl || ii.url,
                fileUrl: ii.url,
                descriptionUrl: ii.descriptionurl,
                license: htmlToText((meta.LicenseShortName && meta.LicenseShortName.value) || ""),
                author: htmlToText((meta.Artist && meta.Artist.value) || ""),
                attribution: htmlToText((meta.AttributionRequired && meta.AttributionRequired.value) || ""),
                credit: htmlToText((meta.Credit && meta.Credit.value) || ""),
            });
        }
        results.sort((a, b) => a.index - b.index);
        // continue offset for "load more"
        const cont = data && data.continue && (data.continue.gsroffset || data.continue["gsroffset"]);
        return { results, nextOffset: typeof cont === "number" ? cont : null };
    }

    async function fetchSvg(title, fileUrl) {
        if (svgCache.has(title)) return svgCache.get(title);
        const r = await fetch(fileUrl);
        if (!r.ok) throw new Error("SVG download failed: HTTP " + r.status);
        const txt = await r.text();
        svgCache.set(title, txt);
        return txt;
    }

    // ─── SVG recolor + fit ───────────────────────────────────────────────
    /**
     * Replace every solid color (fill / stroke, attribute or inline style)
     * with `color`. "none", "transparent" and url(#…) refs are preserved.
     * Elements without any fill/stroke get an explicit fill so they show.
     */
    function recolorSvg(svgText, color) {
        if (!color) return svgText;
        const doc = new DOMParser().parseFromString(svgText, "image/svg+xml");
        if (doc.getElementsByTagName("parsererror").length) return svgText;

        const root = doc.documentElement;
        // Drop any inline <style> blocks that could fight our overrides.
        // (Most Commons SVGs have very simple styling; we keep behavior conservative.)
        const styleNodes = root.querySelectorAll("style");
        styleNodes.forEach((s) => {
            // Replace color tokens inside <style> too.
            s.textContent = (s.textContent || "")
                .replace(/(fill|stroke)\s*:\s*(#[0-9a-fA-F]{3,8}|rgb\([^)]+\)|rgba\([^)]+\)|[a-zA-Z]+)/g,
                    (m, prop, val) => isPreserved(val) ? m : `${prop}:${color}`);
        });

        // Walk every element.
        const walker = doc.createTreeWalker(root, NodeFilter.SHOW_ELEMENT);
        let n = walker.currentNode;
        while (n) {
            applyColorToElement(n, color);
            n = walker.nextNode();
        }
        // Ensure the root paints in our color for elements that didn't declare fill.
        if (!root.hasAttribute("fill") || isPreserved(root.getAttribute("fill"))) {
            // leave preserved values alone
        }
        // Set a default fill on the root so unattributed paths inherit it.
        root.setAttribute("fill", color);

        return new XMLSerializer().serializeToString(doc);
    }

    function isPreserved(val) {
        if (!val) return true;
        const v = String(val).trim().toLowerCase();
        return v === "none" || v === "transparent" || v.startsWith("url(");
    }

    function applyColorToElement(el, color) {
        // Attributes
        for (const attr of ["fill", "stroke"]) {
            const v = el.getAttribute(attr);
            if (v != null && !isPreserved(v)) el.setAttribute(attr, color);
        }
        // Inline style
        const style = el.getAttribute("style");
        if (style) {
            const replaced = style.replace(
                /(fill|stroke)\s*:\s*([^;]+)/gi,
                (m, prop, val) => isPreserved(val) ? m : `${prop}:${color}`
            );
            el.setAttribute("style", replaced);
        }
    }

    function fitSvg(rawSvg, targetW, targetH, padding, fitMode, bgColor) {
        const doc = new DOMParser().parseFromString(rawSvg, "image/svg+xml");
        const src = doc.documentElement;

        let vb = src.getAttribute("viewBox");
        if (!vb) {
            const w = parseFloat(src.getAttribute("width")) || 100;
            const h = parseFloat(src.getAttribute("height")) || 100;
            vb = `0 0 ${w} ${h}`;
        }
        const [vx, vy, vw, vh] = vb.split(/\s+|,/).map(parseFloat);

        const innerW = Math.max(1, targetW - padding * 2);
        const innerH = Math.max(1, targetH - padding * 2);

        let drawW, drawH, scale;
        if (fitMode === "stretch") { drawW = innerW; drawH = innerH; }
        else {
            scale = Math.min(innerW / vw, innerH / vh);
            drawW = vw * scale; drawH = vh * scale;
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

    function svgToBlob(s) { return new Blob([s], { type: "image/svg+xml;charset=utf-8" }); }
    function svgToRaster(svg, w, h, mime, background) {
        return new Promise((resolve, reject) => {
            const url = URL.createObjectURL(svgToBlob(svg));
            const img = new Image();
            img.onload = () => {
                const c = document.createElement("canvas");
                c.width = w; c.height = h;
                const ctx = c.getContext("2d");
                if (background) { ctx.fillStyle = background; ctx.fillRect(0, 0, w, h); }
                ctx.drawImage(img, 0, 0, w, h);
                URL.revokeObjectURL(url);
                c.toBlob((b) => b ? resolve(b) : reject(new Error("toBlob failed")),
                    mime, mime === "image/jpeg" ? 0.95 : undefined);
            };
            img.onerror = () => { URL.revokeObjectURL(url); reject(new Error("rasterize failed")); };
            img.src = url;
        });
    }

    // ─── UI ──────────────────────────────────────────────────────────────
    function makeTile(item, isSelectedGrid) {
        const tile = document.createElement("div");
        tile.className = "icon-tile";
        tile.dataset.title = item.title;
        if (selected.has(item.title)) tile.classList.add("selected");

        const lic = document.createElement("span");
        lic.className = "license";
        lic.textContent = item.license || "?";
        lic.title = [item.license, item.author, item.credit].filter(Boolean).join("\n");
        tile.appendChild(lic);

        if (isSelectedGrid) {
            const rm = document.createElement("button");
            rm.className = "remove";
            rm.type = "button";
            rm.textContent = "×";
            rm.title = "Remove";
            rm.addEventListener("click", (e) => {
                e.stopPropagation();
                selected.delete(item.title);
                renderSelected();
                const t = els.results.querySelector(`[data-title="${cssEscape(item.title)}"]`);
                if (t) t.classList.remove("selected");
            });
            tile.appendChild(rm);
        }

        const wrap = document.createElement("div");
        wrap.className = "img-wrap";
        const img = document.createElement("img");
        img.src = item.thumbUrl;
        img.alt = item.title;
        img.loading = "lazy";
        wrap.appendChild(img);
        tile.appendChild(wrap);

        const lab = document.createElement("div");
        lab.className = "label";
        lab.textContent = item.title.replace(/^File:/, "").replace(/\.svg$/i, "");
        lab.title = item.title + (item.descriptionUrl ? "\n" + item.descriptionUrl : "");
        tile.appendChild(lab);

        if (!isSelectedGrid) {
            tile.addEventListener("click", () => {
                if (selected.has(item.title)) selected.delete(item.title);
                else selected.set(item.title, item);
                tile.classList.toggle("selected", selected.has(item.title));
                renderSelected();
            });
        }
        return tile;
    }

    function cssEscape(s) { return s.replace(/["\\]/g, "\\$&"); }

    function renderSelected() {
        els.selected.innerHTML = "";
        if (!selected.size) {
            els.selected.innerHTML = '<p class="hint" style="grid-column:1/-1;margin:0">Nothing selected yet. Click results above to add them.</p>';
            return;
        }
        for (const item of selected.values()) {
            els.selected.appendChild(makeTile(item, true));
        }
    }

    function buildFullQuery() {
        const q = els.query.value.trim();
        if (!q) return "";
        const suffix = els.preset.value;
        return suffix ? `${q} ${suffix}` : q;
    }

    async function runSearch(reset) {
        if (reset) {
            searchOffset = 0;
            els.results.innerHTML = "";
            lastFullQuery = buildFullQuery();
        }
        if (!lastFullQuery) { els.searchStatus.textContent = "Type something to search."; return; }

        els.searchBtn.disabled = true;
        els.searchStatus.textContent = "Searching Commons…";
        try {
            const { results, nextOffset } = await searchCommons(lastFullQuery, 30, searchOffset);
            for (const item of results) {
                els.results.appendChild(makeTile(item, false));
            }
            if (nextOffset != null) { searchOffset = nextOffset; els.loadMore.hidden = false; }
            else { els.loadMore.hidden = true; }
            if (!els.results.childElementCount) {
                els.results.innerHTML = '<p class="hint" style="grid-column:1/-1">No SVG results. Try a more specific or alternate term.</p>';
            }
            els.searchStatus.textContent = `Showing ${els.results.childElementCount} SVG results`;
        } catch (e) {
            els.searchStatus.textContent = "Error: " + (e.message || e);
        } finally {
            els.searchBtn.disabled = false;
        }
    }

    // ─── Download ────────────────────────────────────────────────────────
    async function renderOne(item) {
        const w = clampInt(els.width.value, 8, 4096, 256);
        const h = clampInt(els.height.value, 8, 4096, 256);
        const padding = clampInt(els.padding.value, 0, 512, 0);
        const fitMode = els.fitMode.value;
        const format = els.format.value;
        const color = currentColor();
        const bg = currentBg();

        let svg = await fetchSvg(item.title, item.fileUrl);
        if (color) svg = recolorSvg(svg, color);
        const finalSvg = fitSvg(svg, w, h, padding, fitMode, bg);

        let blob, ext;
        if (format === "svg") { blob = svgToBlob(finalSvg); ext = "svg"; }
        else if (format === "png") { blob = await svgToRaster(finalSvg, w, h, "image/png", null); ext = "png"; }
        else { blob = await svgToRaster(finalSvg, w, h, "image/jpeg", bg || "#ffffff"); ext = "jpg"; }
        return { item, blob, ext };
    }

    function downloadBlob(blob, filename) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url; a.download = filename;
        document.body.appendChild(a); a.click(); a.remove();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    }

    function buildAttribution(items) {
        const lines = [
            "Files sourced from Wikimedia Commons (https://commons.wikimedia.org).",
            "Each file below carries the license stated. CC-BY / CC-BY-SA files require credit.",
            "",
        ];
        for (const it of items) {
            lines.push(`* ${it.title}`);
            if (it.license) lines.push(`  License : ${it.license}`);
            if (it.author)  lines.push(`  Author  : ${it.author}`);
            if (it.credit)  lines.push(`  Credit  : ${it.credit}`);
            if (it.descriptionUrl) lines.push(`  Source  : ${it.descriptionUrl}`);
            lines.push("");
        }
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
            for (const item of selected.values()) {
                try { results.push(await renderOne(item)); }
                catch (e) { errors.push(`${item.title}: ${e.message || e}`); }
            }
            const fnFor = (r) => sanitizeFileName(r.item.title.replace(/\.svg$/i, "")) + suffix + "." + r.ext;

            if (results.length === 1) {
                // Even one file: zip if license isn't CC0/PD, so the credit comes along.
                const single = results[0];
                const needsAttr = !isPublicDomain(single.item.license);
                if (!needsAttr) {
                    downloadBlob(single.blob, fnFor(single));
                } else {
                    const zip = new JSZip();
                    zip.file(fnFor(single), single.blob);
                    zip.file("ATTRIBUTION.txt", buildAttribution(results.map((r) => r.item)));
                    const z = await zip.generateAsync({ type: "blob" });
                    downloadBlob(z, sanitizeFileName(single.item.title.replace(/\.svg$/i, "")) + ".zip");
                }
            } else if (results.length > 1) {
                const zip = new JSZip();
                for (const r of results) zip.file(fnFor(r), r.blob);
                zip.file("ATTRIBUTION.txt", buildAttribution(results.map((r) => r.item)));
                const z = await zip.generateAsync({ type: "blob" });
                const stamp = new Date().toISOString().replace(/[:.]/g, "-").slice(0, 19);
                downloadBlob(z, `silhouettes_${stamp}.zip`);
            }

            const msg = [];
            if (results.length) msg.push(`Exported ${results.length}`);
            if (errors.length) msg.push(`${errors.length} failed`);
            els.status.textContent = msg.join(", ") + (errors.length ? " — " + errors.join("; ") : "");
        } finally {
            els.downloadAll.disabled = false;
        }
    }

    function isPublicDomain(license) {
        if (!license) return false;
        const l = license.toLowerCase();
        return l.includes("cc0") || l.includes("public domain") || l.includes("pd");
    }

    // ─── Wire up ─────────────────────────────────────────────────────────
    els.searchBtn.addEventListener("click", () => runSearch(true));
    els.query.addEventListener("keydown", (e) => {
        if (e.key === "Enter") { e.preventDefault(); runSearch(true); }
    });
    els.preset.addEventListener("change", () => runSearch(true));
    els.loadMore.addEventListener("click", () => runSearch(false));
    els.downloadAll.addEventListener("click", onDownloadAll);
    els.clearSelected.addEventListener("click", () => {
        selected.clear();
        renderSelected();
        els.results.querySelectorAll(".icon-tile.selected").forEach((t) => t.classList.remove("selected"));
    });

    renderSelected();
    // No auto-search on load (Commons API is slow-ish and the user has a specific query).
    els.searchStatus.textContent = "Try: 'tank', 'rifle', 'F-16', 'destroyer', 'soldier'…";
})();
