/* font tracker — bake letter-spacing into a font by shifting hmtx advance widths.
 *
 * Tracking is expressed in 1/1000 em (the typographic convention) so a single
 * value behaves identically at every point size. It converts to font units as
 *     delta = round(perMille / 1000 * unitsPerEm)
 * and is added to every non-zero glyph advance. Zero-advance glyphs are
 * combining marks — widening those would detach accents from their base.
 */
(function () {
    'use strict';

    const $ = (id) => document.getElementById(id);

    const state = {
        raw: null,        // ArrayBuffer of the file as uploaded
        font: null,       // opentype.Font
        baseAdv: null,    // original advance widths, so edits never compound
        upm: 1000,
        baseName: 'font',
        origFamily: null,
        newFamily: null,
        origFace: null,
        newFace: null,
        outBuf: null,
        seq: 0,
        tracking: -20,    // 1/1000 em — source of truth, not the slider
        clamped: 0,       // glyphs driven to zero advance by extreme tightening
        narrowest: 0,
    };

    const measureCanvas = document.createElement('canvas').getContext('2d');

    /* ---------- loading ---------- */

    function showError(msg) {
        const el = $('load-error');
        el.textContent = msg;
        el.classList.remove('hidden');
    }

    function clearError() { $('load-error').classList.add('hidden'); }

    async function loadFile(file) {
        clearError();
        state.baseName = file.name.replace(/\.[^.]+$/, '');

        let buf;
        try {
            buf = await file.arrayBuffer();
        } catch (e) {
            showError('Could not read the file: ' + e.message);
            return;
        }

        let font;
        try {
            font = opentype.parse(buf);
        } catch (e) {
            showError('opentype.js could not parse this font: ' + e.message);
            return;
        }

        state.raw = buf;
        state.font = font;
        state.upm = font.unitsPerEm || 1000;

        const n = font.glyphs.length;
        state.baseAdv = new Array(n);
        for (let i = 0; i < n; i++) {
            const g = font.glyphs.get(i);
            state.baseAdv[i] = (typeof g.advanceWidth === 'number') ? g.advanceWidth : 0;
        }

        const names = font.names || {};
        const pick = (rec) => (rec && (rec.en || Object.values(rec)[0])) || '—';
        $('f-family').textContent = pick(names.fontFamily);
        $('f-style').textContent = pick(names.fontSubfamily);
        $('f-upm').textContent = state.upm;
        $('f-glyphs').textContent = n;
        const cff = font.outlinesFormat === 'cff';
        $('f-outlines').textContent = cff ? 'CFF (PostScript)' : 'TrueType (glyf)';
        $('facts').classList.remove('hidden');
        $('cff-warn').classList.toggle('hidden', !cff);

        // the untouched original, for side-by-side comparison
        if (state.origFace) { document.fonts.delete(state.origFace); }
        state.origFamily = 'ft-orig-' + (++state.seq);
        state.origFace = new FontFace(state.origFamily, state.raw.slice(0));
        try {
            await state.origFace.load();
            document.fonts.add(state.origFace);
        } catch (e) {
            showError('The browser refused to load this font for preview: ' + e.message);
            return;
        }

        regenerate();
    }

    /* ---------- patching ---------- */

    function trackingValue() { return state.tracking; }

    function deltaUnits() { return Math.round(trackingValue() / 1000 * state.upm); }

    /* The slider is a convenience, not a limit: a typed value is honoured as-is
     * and the slider simply pins to whichever end it ran past. */
    function setTracking(v, source) {
        if (!Number.isFinite(v)) { return; }
        state.tracking = Math.round(v);
        const slider = $('track');
        if (source !== 'slider') {
            slider.value = Math.max(Number(slider.min), Math.min(Number(slider.max), state.tracking));
        }
        if (source !== 'number') { $('track-num').value = state.tracking; }
    }

    async function regenerate() {
        if (!state.font) { return; }

        const delta = deltaUnits();
        const font = state.font;
        let clamped = 0;
        let narrowest = Infinity;
        for (let i = 0; i < font.glyphs.length; i++) {
            const base = state.baseAdv[i];
            if (base === 0) { continue; }            // combining mark — leave it
            const want = base + delta;
            if (want < 0) { clamped++; }             // can't advance backwards
            const adv = Math.max(0, want);
            if (adv < narrowest) { narrowest = adv; }
            font.glyphs.get(i).advanceWidth = adv;
        }
        state.clamped = clamped;
        state.narrowest = Number.isFinite(narrowest) ? narrowest : 0;

        let buf;
        try {
            buf = font.toArrayBuffer();
        } catch (e) {
            showError('Could not re-serialise the font: ' + e.message);
            return;
        }
        state.outBuf = buf;

        if (state.newFace) { document.fonts.delete(state.newFace); }
        state.newFamily = 'ft-new-' + (++state.seq);
        state.newFace = new FontFace(state.newFamily, buf.slice(0));
        try {
            await state.newFace.load();
            document.fonts.add(state.newFace);
        } catch (e) {
            showError('The patched font failed to load for preview: ' + e.message);
            return;
        }

        $('download').disabled = false;
        $('out-name').textContent = outputName();
        refresh();
    }

    function outputName() {
        const v = trackingValue();
        const tag = v === 0 ? '0' : (v > 0 ? 'p' + v : 'm' + Math.abs(v));
        return state.baseName + '-track' + tag + '.ttf';
    }

    /* ---------- preview ---------- */

    function widestLine(text, family, sizePx) {
        measureCanvas.font = sizePx + 'px "' + family + '"';
        let max = 0;
        for (const line of text.split('\n')) {
            max = Math.max(max, measureCanvas.measureText(line).width);
        }
        return max;
    }

    function refresh() {
        const size = parseInt($('size').value, 10);
        const fieldW = parseInt($('fieldw').value, 10);
        const text = $('sample').value || ' ';

        $('size-val').textContent = size + ' px';
        $('fieldw-val').textContent = fieldW === 0 ? 'off' : fieldW + ' px';

        const delta = deltaUnits();
        $('f-delta').textContent = delta + ' units';
        $('f-px16').textContent = (delta / state.upm * 16).toFixed(2) + ' px / glyph';
        $('f-narrow').textContent = state.font ? state.narrowest + ' units' : '—';

        const cw = $('clamp-warn');
        if (state.clamped > 0) {
            cw.textContent = state.clamped + ' glyph' + (state.clamped === 1 ? '' : 's') +
                ' hit zero advance and were clamped — a glyph cannot advance backwards, so those ' +
                'letters now sit directly on top of the next one. Everything still exports, but ' +
                'spacing past this point is no longer uniform.';
            cw.classList.remove('hidden');
        } else {
            cw.classList.add('hidden');
        }

        const oldEl = $('prev-orig');
        const newEl = $('prev-new');
        oldEl.textContent = text;
        newEl.textContent = text;

        if (state.origFamily) { oldEl.style.font = size + 'px "' + state.origFamily + '"'; }
        if (state.newFamily) { newEl.style.font = size + 'px "' + state.newFamily + '"'; }

        for (const el of [oldEl, newEl]) {
            el.classList.toggle('guided', fieldW > 0);
            el.style.setProperty('--field-w', fieldW + 'px');
        }

        if (!state.origFamily || !state.newFamily) { return; }

        const wOrig = widestLine(text, state.origFamily, size);
        const wNew = widestLine(text, state.newFamily, size);
        $('w-orig').textContent = wOrig.toFixed(1) + ' px';
        $('w-new').textContent = wNew.toFixed(1) + ' px';

        const diff = wNew - wOrig;
        const pct = wOrig > 0 ? (diff / wOrig * 100) : 0;
        const d = $('w-delta');
        d.textContent = (diff <= 0 ? '' : '+') + diff.toFixed(1) + ' px (' +
            (pct <= 0 ? '' : '+') + pct.toFixed(1) + '%)';
        d.classList.toggle('worse', diff > 0);

        const note = $('fit-note');
        if (fieldW === 0) {
            note.innerHTML = '&nbsp;';
        } else {
            const fitO = wOrig <= fieldW;
            const fitN = wNew <= fieldW;
            if (fitN && !fitO) {
                note.textContent = 'Fits at this tracking — the original overflows by ' +
                    (wOrig - fieldW).toFixed(1) + ' px.';
            } else if (fitN) {
                note.textContent = 'Both fit. ' + (fieldW - wNew).toFixed(1) + ' px to spare.';
            } else {
                note.textContent = 'Still overflows by ' + (wNew - fieldW).toFixed(1) +
                    ' px — tighten further, or drop a point size.';
            }
        }
    }

    /* ---------- wiring ---------- */

    let regenTimer = null;
    function scheduleRegen() {
        refresh();                                   // instant readout
        clearTimeout(regenTimer);
        regenTimer = setTimeout(regenerate, 120);    // rebuild is the slow part
    }

    $('track').addEventListener('input', () => {
        setTracking(parseInt($('track').value, 10), 'slider');
        scheduleRegen();
    });
    $('track-num').addEventListener('input', () => {
        const v = parseInt($('track-num').value, 10);
        if (!Number.isFinite(v)) { return; }     // mid-typing "-" or an empty box
        setTracking(v, 'number');
        scheduleRegen();
    });
    for (const b of document.querySelectorAll('[data-preset]')) {
        b.addEventListener('click', () => {
            setTracking(parseInt(b.dataset.preset, 10));
            scheduleRegen();
        });
    }

    $('size').addEventListener('input', refresh);
    $('fieldw').addEventListener('input', refresh);
    $('sample').addEventListener('input', refresh);

    $('file').addEventListener('change', (e) => {
        if (e.target.files[0]) { loadFile(e.target.files[0]); }
    });

    const drop = $('drop');
    drop.addEventListener('dragover', (e) => { e.preventDefault(); drop.classList.add('over'); });
    drop.addEventListener('dragleave', () => drop.classList.remove('over'));
    drop.addEventListener('drop', (e) => {
        e.preventDefault();
        drop.classList.remove('over');
        if (e.dataTransfer.files[0]) { loadFile(e.dataTransfer.files[0]); }
    });

    $('download').addEventListener('click', () => {
        if (!state.outBuf) { return; }
        const url = URL.createObjectURL(new Blob([state.outBuf], { type: 'font/ttf' }));
        const a = document.createElement('a');
        a.href = url;
        a.download = outputName();
        a.click();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    });

    refresh();
})();
