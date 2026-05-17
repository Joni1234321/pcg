"use strict";
// ═══════════════════════════════════════════════════════════
//  BATTLES — Regiment Combat on a 5-deep Hex Grid
// ═══════════════════════════════════════════════════════════
//
//      [A1] [A2] [A3]            row 0  DEFENDER REAR
//   [B1] [B2] [B3] [B4]         row 1  MAIN DEFENCE LINE
//      [C1] [C2] [C3]            row 2  NO MAN'S LAND
//   [D1] [D2] [D3] [D4]         row 3  APPROACH
//      [E1] [E2] [E3]            row 4  ATTACKER START
//
//  Off-map: 394th Art Bn (12×105mm)  /  1028th Art Btry (8×76mm)
//  Flanking fire from adjacent unhit defenders.
//  Entrenched defenders get ×1.3 bonus (lost on movement).
//  Firepower ≠ Manpower.  MGs & mortars multiply damage.
// ═══════════════════════════════════════════════════════════
// ── Grid constants ───────────────────────────────────────
const LABELS = [
    "A1", "A2", "A3",
    "B1", "B2", "B3", "B4",
    "C1", "C2", "C3",
    "D1", "D2", "D3", "D4",
    "E1", "E2", "E3",
];
const N_TILES = 17;
const ADJ = [
    /*  0 A1 */ [1, 3, 4],
    /*  1 A2 */ [0, 2, 4, 5],
    /*  2 A3 */ [1, 5, 6],
    /*  3 B1 */ [0, 4, 7],
    /*  4 B2 */ [0, 1, 3, 5, 7, 8],
    /*  5 B3 */ [1, 2, 4, 6, 8, 9],
    /*  6 B4 */ [2, 5, 9],
    /*  7 C1 */ [3, 4, 8, 10, 11],
    /*  8 C2 */ [4, 5, 7, 9, 11, 12],
    /*  9 C3 */ [5, 6, 8, 12, 13],
    /* 10 D1 */ [7, 11, 14],
    /* 11 D2 */ [7, 8, 10, 12, 14, 15],
    /* 12 D3 */ [8, 9, 11, 13, 15, 16],
    /* 13 D4 */ [9, 12, 16],
    /* 14 E1 */ [10, 11, 15],
    /* 15 E2 */ [11, 12, 14, 16],
    /* 16 E3 */ [12, 13, 15],
];
const ROW = [0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4];
const ZONES = ["DEFENDER REAR", "MAIN LINE", "NO MAN'S LAND", "APPROACH", "ATTACKER START"];
const TERRAINS = [
    { name: "field", mul: 1.0, icon: "━" },
    { name: "hill", mul: 1.5, icon: "△" },
    { name: "village", mul: 1.8, icon: "⌂" },
    { name: "forest", mul: 2.0, icon: "♣" },
    { name: "ridge", mul: 1.7, icon: "∧" },
];
const SUPP = ["READY", "DISRUPTED", "SUPPRESSED", "PINNED"];
const PH = ["DEPLOY", "RECON", "ARTILLERY", "ASSAULT", "MOVEMENT"];
// hex layout
const HW = 100, HH = 115;
const CS = HW + 4, RS = Math.floor(HH * .75) + 4, HO = Math.floor(CS / 2);
const CLIP = "polygon(50% 0%,100% 25%,100% 75%,50% 100%,0% 75%,0% 25%)";
// off-map box offset
const MAP_PAD_TOP = 36;
const HP = [];
{
    const rows = [[0, 1, 2], [3, 4, 5, 6], [7, 8, 9], [10, 11, 12, 13], [14, 15, 16]];
    for (let r = 0; r < 5; r++) {
        const off = rows[r].length === 3 ? HO : 0;
        for (let c = 0; c < rows[r].length; c++) {
            HP[rows[r][c]] = { x: off + c * CS, y: MAP_PAD_TOP + r * RS };
        }
    }
}
// ── State ────────────────────────────────────────────────
let tiles = [];
let bats = [];
let fullLog = [];
let turn = 0, phase = 0, over = false;
let atkGuns = 12, defGuns = 8;
let steps = [];
let stepIdx = 0;
// ── Helpers ──────────────────────────────────────────────
function rand(a, b) {
    if (b === undefined)
        return Math.floor(Math.random() * a);
    return a + Math.floor(Math.random() * (b - a + 1));
}
function d6() { return rand(1, 6); }
function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }
function $(id) { return document.getElementById(id); }
function esc(s) { const el = document.createElement("span"); el.textContent = s; return el.innerHTML; }
function addLog(t, ty = "info") { fullLog.push({ text: t, type: ty }); }
function fp(b) {
    return Math.round(b.rifles / 10 + b.mg + b.mortar * 0.67);
}
function applyCas(b, n) {
    const actual = Math.min(n, Math.max(0, b.men));
    b.men -= actual;
    b.rifles = Math.max(0, b.rifles - Math.floor(actual * 0.85));
    if (actual >= 25)
        b.mg = Math.max(0, b.mg - Math.floor(actual / 70));
    if (actual >= 50 && b.mortar > 0 && d6() <= 2)
        b.mortar--;
}
function snap() {
    return { turn, phase, bats: bats.map(b => ({ ...b })), atkGuns, defGuns, logEnd: fullLog.length, over };
}
// ── Init & Simulate ──────────────────────────────────────
function initBattle() {
    // terrain pool (17 tiles): open ground at edges, tougher middle
    const pool = [
        "field", "village", "field",
        "forest", "hill", "village", "ridge",
        "field", "field", "field",
        "field", "hill", "field", "field",
        "field", "field", "field",
    ];
    for (let i = pool.length - 1; i > 0; i--) {
        const j = rand(i + 1);
        [pool[i], pool[j]] = [pool[j], pool[i]];
    }
    tiles = pool.map((t, i) => {
        const info = TERRAINS.find(x => x.name === t);
        return { index: i, terrain: t, mul: info.mul, icon: info.icon };
    });
    // All attackers start on E2 (tile 15) — concentrated thrust
    bats = [
        { id: "I/394", bn: "I", rgt: "394th", side: "atk", men: 820, rifles: 700, mg: 12, mortar: 6, morale: 85, suppression: 0, tile: 15, revealed: true, routed: false, entrenched: false },
        { id: "II/394", bn: "II", rgt: "394th", side: "atk", men: 790, rifles: 670, mg: 12, mortar: 6, morale: 80, suppression: 0, tile: 15, revealed: true, routed: false, entrenched: false },
        { id: "III/394", bn: "III", rgt: "394th", side: "atk", men: 850, rifles: 740, mg: 14, mortar: 8, morale: 90, suppression: 0, tile: 15, revealed: true, routed: false, entrenched: false },
        { id: "I/1028", bn: "I", rgt: "1028th", side: "def", men: 680, rifles: 580, mg: 8, mortar: 4, morale: 70, suppression: 0, tile: 3, revealed: false, routed: false, entrenched: true },
        { id: "II/1028", bn: "II", rgt: "1028th", side: "def", men: 720, rifles: 620, mg: 10, mortar: 6, morale: 75, suppression: 0, tile: 5, revealed: false, routed: false, entrenched: true },
        { id: "III/1028", bn: "III", rgt: "1028th", side: "def", men: 650, rifles: 560, mg: 8, mortar: 4, morale: 65, suppression: 0, tile: 6, revealed: false, routed: false, entrenched: true },
    ];
    // one defender forward on C-line as outpost
    const defIdx = rand(3);
    bats[3 + defIdx].tile = [7, 8, 9][rand(3)];
    bats[3 + defIdx].entrenched = false; // outpost, not dug in
    atkGuns = 12;
    defGuns = 8;
    turn = 1;
    phase = 0;
    over = false;
    fullLog = [];
    simulateAll();
    stepIdx = 0;
    renderFrame();
}
function simulateAll() {
    steps = [];
    const atk = bats.filter(b => b.side === "atk");
    const def = bats.filter(b => b.side === "def");
    addLog("══ 394th Infantry Rgt  vs  1028th Rifle Rgt ══", "hdr");
    addLog(`394th: ${atk.reduce((s, b) => s + b.men, 0)} men  ⚔${atk.reduce((s, b) => s + fp(b), 0)} FP  │  Off-map: ${atkGuns}× 105mm`, "info");
    addLog(`1028th: ${def.reduce((s, b) => s + b.men, 0)} men  ⚔${def.reduce((s, b) => s + fp(b), 0)} FP  │  Off-map: ${defGuns}× 76mm  │  ENTRENCHED`, "info");
    addLog("Objective: breach the main line and reach the A-row.", "info");
    steps.push(snap());
    while (!over) {
        phase++;
        if (phase > 4) {
            phase = 1;
            turn++;
            for (const b of bats)
                if (!b.routed)
                    b.suppression = Math.max(0, b.suppression - 1);
            addLog(`\n════ TURN ${turn} ════`, "hdr");
        }
        switch (phase) {
            case 1:
                doRecon();
                break;
            case 2:
                doArtillery();
                break;
            case 3:
                doAssault();
                break;
            case 4:
                doMovement();
                break;
        }
        checkEnd();
        steps.push(snap());
    }
}
// ── 1. RECON ─────────────────────────────────────────────
function doRecon() {
    addLog("── Recon  0500–0700 ──", "hdr");
    for (const a of bats.filter(b => b.side === "atk" && !b.routed)) {
        for (const ti of ADJ[a.tile]) {
            const hidden = bats.filter(b => b.side === "def" && b.tile === ti && !b.routed && !b.revealed);
            for (const d of hidden) {
                const roll = d6();
                const pen = tiles[ti].terrain === "forest" ? 2 : tiles[ti].terrain === "village" ? 1 : 0;
                if (roll - pen >= 4) {
                    d.revealed = true;
                    const ent = d.entrenched ? " [ENTRENCHED]" : "";
                    addLog(`${a.id} → ${LABELS[ti]}: SPOTTED ${d.id} (~${Math.round(d.men / 50) * 50} men, ${d.mg}MG)${ent}`, "recon");
                }
                else if (roll === 1) {
                    const l = rand(5, 15);
                    applyCas(a, l);
                    addLog(`${a.id} ambushed near ${LABELS[ti]}! −${l} men`, "combat");
                }
                else {
                    addLog(`${a.id} → ${LABELS[ti]}: clear`, "recon");
                }
            }
        }
    }
}
// ── 2. ARTILLERY (off-map) ───────────────────────────────
function doArtillery() {
    addLog("── Artillery  0700–0900 ──", "hdr");
    // ── ATTACKER: 394th Art Bn ──
    if (atkGuns > 0) {
        const tgt = new Set();
        for (const b of bats)
            if (b.side === "def" && b.revealed && !b.routed)
                tgt.add(b.tile);
        if (tgt.size === 0) {
            addLog(`394th Art Bn (${atkGuns}× 105mm): no targets spotted`, "arty");
        }
        else {
            const targets = [...tgt];
            const perTgt = Math.ceil(atkGuns / targets.length);
            let used = 0;
            for (const ti of targets) {
                const guns = Math.min(perTgt, atkGuns - used);
                used += guns;
                for (const d of bats.filter(b => b.side === "def" && b.tile === ti && !b.routed)) {
                    let cas = 0;
                    for (let g = 0; g < guns; g++)
                        cas += Math.floor(rand(3, 8) / tiles[ti].mul);
                    applyCas(d, cas);
                    const sl = guns >= 7 ? 3 : guns >= 4 ? 2 : guns >= 2 ? 1 : 0;
                    d.suppression = Math.max(d.suppression, sl);
                    d.morale = clamp(d.morale - rand(3, 7), 0, 100);
                    addLog(`394th Art (${guns}× 105mm) → ${LABELS[ti]}: ${d.id} −${cas} men, ${SUPP[d.suppression]}`, "arty");
                }
            }
        }
    }
    // ── DEFENDER: 1028th Art Btry ──
    if (defGuns > 0) {
        const atkTiles = [];
        for (const b of bats)
            if (b.side === "atk" && !b.routed && !atkTiles.includes(b.tile))
                atkTiles.push(b.tile);
        if (atkTiles.length > 0) {
            const ti = atkTiles[rand(atkTiles.length)];
            const targets = bats.filter(b => b.side === "atk" && b.tile === ti && !b.routed);
            for (const a of targets) {
                const cas = Math.max(1, Math.floor(rand(2, 5) * defGuns / Math.max(1, targets.length) / tiles[ti].mul));
                applyCas(a, cas);
                addLog(`1028th Art (${defGuns}× 76mm) → ${LABELS[ti]}: ${a.id} −${cas} men`, "arty");
            }
        }
    }
    // counter-battery
    if (atkGuns > 0 && defGuns > 0 && d6() >= 5) {
        defGuns--;
        addLog(`Counter-battery: 1028th Art loses 1 gun (${defGuns} left)`, "arty");
    }
    if (defGuns > 0 && atkGuns > 0 && d6() >= 5) {
        atkGuns--;
        addLog(`Counter-battery: 394th Art loses 1 gun (${atkGuns} left)`, "arty");
    }
}
// ── 3. ASSAULT ───────────────────────────────────────────
function doAssault() {
    addLog("── Assault  0900–1100 ──", "hdr");
    const plan = new Map();
    for (const a of bats.filter(b => b.side === "atk" && !b.routed)) {
        // can attack forward-adjacent tiles with revealed defenders
        const targets = ADJ[a.tile]
            .filter(t => ROW[t] < ROW[a.tile])
            .filter(t => bats.some(b => b.side === "def" && b.tile === t && !b.routed && b.revealed));
        // also contest same tile
        if (bats.some(b => b.side === "def" && b.tile === a.tile && !b.routed))
            targets.push(a.tile);
        if (!targets.length)
            continue;
        const best = targets.reduce((a2, b2) => {
            const s = (ti) => bats.filter(b => b.side === "def" && b.tile === ti && !b.routed).reduce((x, b) => x + fp(b), 0);
            return s(b2) < s(a2) ? b2 : a2;
        });
        if (!plan.has(best))
            plan.set(best, []);
        plan.get(best).push(a);
    }
    if (plan.size === 0) {
        addLog("No engagements.", "info");
        return;
    }
    const attackedTiles = new Set(plan.keys());
    for (const [ti, atks] of plan) {
        const ds = bats.filter(b => b.side === "def" && b.tile === ti && !b.routed);
        if (ds.length > 0)
            resolveCombat(atks, ds, ti, attackedTiles);
    }
}
function resolveCombat(atks, defs, ti, attackedTiles) {
    const tile = tiles[ti];
    const atkFP = atks.reduce((s, a) => s + fp(a), 0);
    const defFP = defs.reduce((s, d) => s + fp(d), 0);
    if (atkFP <= 0 || defFP <= 0)
        return;
    // ── flanking fire from adjacent un-attacked defenders ──
    let flankFP = 0;
    const flankFrom = [];
    for (const adj of ADJ[ti]) {
        if (attackedTiles.has(adj))
            continue;
        for (const fd of bats.filter(b => b.side === "def" && b.tile === adj && !b.routed)) {
            const contrib = Math.floor(fp(fd) * 0.3);
            if (contrib > 0) {
                flankFP += contrib;
                flankFrom.push(LABELS[adj]);
            }
        }
    }
    const totalDefFP = defFP + flankFP;
    if (flankFP > 0)
        addLog(`  flanking fire from ${[...new Set(flankFrom)].join(",")}: +⚔${flankFP}`, "combat");
    // ── entrenchment ──
    const entMul = defs.some(d => d.entrenched) ? 1.3 : 1.0;
    if (entMul > 1)
        addLog(`  defenders entrenched: ×${entMul} bonus`, "combat");
    const tAmen = atks.reduce((s, a) => s + a.men, 0);
    const rawRatio = atkFP / totalDefFP;
    const avgS = defs.reduce((s, d) => s + d.suppression, 0) / defs.length;
    const eff = rawRatio / (tile.mul * entMul) + avgS * 0.15;
    let ar, dr;
    if (eff >= 3.0) {
        ar = 0.015;
        dr = 0.10;
    }
    else if (eff >= 2.0) {
        ar = 0.03;
        dr = 0.07;
    }
    else if (eff >= 1.5) {
        ar = 0.05;
        dr = 0.05;
    }
    else if (eff >= 1.0) {
        ar = 0.07;
        dr = 0.03;
    }
    else {
        ar = 0.09;
        dr = 0.015;
    }
    const aLoss = Math.max(1, Math.floor(tAmen * ar * (0.7 + Math.random() * 0.6)));
    for (const a of atks) {
        const l = Math.max(1, Math.round(aLoss * (a.men / Math.max(1, tAmen))));
        applyCas(a, l);
        a.morale = clamp(a.morale - Math.ceil(l / 10), 0, 100);
        if (a.morale <= 15 || a.men < 80) {
            a.routed = true;
            addLog(`  ✗ ${a.id} ROUTS (${a.men} men)`, "result");
        }
    }
    let dLoss = 0;
    for (const d of defs) {
        const l = Math.max(1, Math.floor(d.men * dr * (0.7 + Math.random() * 0.6)));
        applyCas(d, l);
        d.morale = clamp(d.morale - Math.ceil(l / 8), 0, 100);
        dLoss += l;
        if (d.morale <= 20 || d.men < 100) {
            d.routed = true;
            addLog(`  ✗ ${d.id} ROUTS (${d.men} flee)`, "result");
        }
    }
    const alive = atks.filter(a => !a.routed);
    const dLeft = defs.filter(d => !d.routed);
    let out;
    if (!dLeft.length && alive.length) {
        for (const a of alive)
            a.tile = ti;
        out = "TAKES " + LABELS[ti];
    }
    else if (eff >= 2.0 && alive.length) {
        for (const a of alive)
            a.tile = ti;
        out = "pushes into " + LABELS[ti];
    }
    else if (eff >= 1.4 && d6() >= 4 && alive.length) {
        for (const a of alive)
            a.tile = ti;
        out = "fights into " + LABELS[ti];
    }
    else {
        out = "repelled from " + LABELS[ti];
    }
    addLog(`${atks.map(a => a.id).join("+")} → ${LABELS[ti]}: ${out}  [⚔${rawRatio.toFixed(1)}:1 ter×${(tile.mul * entMul).toFixed(1)} → ${eff.toFixed(1)} eff]  A−${aLoss} D−${dLoss}`, "combat");
}
// ── 4. MOVEMENT ──────────────────────────────────────────
function doMovement() {
    addLog("── Movement  1100–1200 ──", "hdr");
    let moved = false;
    for (const a of bats.filter(b => b.side === "atk" && !b.routed)) {
        if (bats.some(b => b.side === "def" && b.tile === a.tile && !b.routed))
            continue;
        const fw = ADJ[a.tile].filter(t => ROW[t] < ROW[a.tile]);
        if (!fw.length)
            continue;
        // spread out: prefer tiles with fewer defenders, then fewer friendlies
        fw.sort((x, y) => {
            const dx = bats.filter(b => b.side === "def" && b.tile === x && !b.routed).length;
            const dy = bats.filter(b => b.side === "def" && b.tile === y && !b.routed).length;
            if (dx !== dy)
                return dx - dy;
            const fx = bats.filter(b => b.side === "atk" && b.tile === x && !b.routed).length;
            const fy = bats.filter(b => b.side === "atk" && b.tile === y && !b.routed).length;
            return fx - fy;
        });
        const from = a.tile;
        a.tile = fw[0];
        addLog(`${a.id} advances ${LABELS[from]} → ${LABELS[a.tile]}`, "move");
        moved = true;
    }
    for (const d of bats.filter(b => b.side === "def" && !b.routed && b.morale < 35)) {
        const bk = ADJ[d.tile].filter(t => ROW[t] < ROW[d.tile]);
        if (!bk.length)
            continue;
        const from = d.tile;
        d.tile = bk[rand(bk.length)];
        d.entrenched = false; // lost entrenchment by retreating
        addLog(`${d.id} retreats ${LABELS[from]} → ${LABELS[d.tile]} (M:${d.morale}, lost entrenchment)`, "move");
        moved = true;
    }
    if (!moved)
        addLog("No movement.", "info");
}
// ── Victory ──────────────────────────────────────────────
function checkEnd() {
    const ab = bats.filter(b => b.side === "atk" && !b.routed && ROW[b.tile] === 0);
    const da = bats.filter(b => b.side === "def" && !b.routed);
    const aa = bats.filter(b => b.side === "atk" && !b.routed);
    if (ab.length) {
        over = true;
        addLog(`\n★ ATTACKER VICTORY — ${ab[0].id} reached ${LABELS[ab[0].tile]}`, "result");
    }
    else if (!da.length) {
        over = true;
        addLog("\n★ ATTACKER VICTORY — all defenders routed", "result");
    }
    else if (!aa.length) {
        over = true;
        addLog("\n★ DEFENDER VICTORY — all attackers routed", "result");
    }
    else if (turn >= 10) {
        over = true;
        addLog(`\n★ STALEMATE — ${turn} turns`, "result");
    }
}
// ═══════════════════════════════════════════════════════════
//  RENDERING
// ═══════════════════════════════════════════════════════════
function renderFrame() {
    const f = steps[stepIdx];
    renderGrid(f);
    renderOOB(f.bats);
    renderLog(f.logEnd);
    renderPhaseBar(f);
    renderControls();
    const res = $("results");
    if (stepIdx === steps.length - 1 && f.over) {
        renderResults();
        res.classList.remove("hidden");
    }
    else
        res.classList.add("hidden");
}
// ── Hex grid + off-map ───────────────────────────────────
function renderGrid(f) {
    const g = $("hex-grid");
    let html = "";
    // ── off-map defender art (top) ──
    html += `<div class="offmap offmap-def">`
        + `<span class="offmap-lbl">OFF-MAP</span>`
        + `<span class="offmap-nm">1028th Art Btry</span>`
        + `<span class="offmap-guns">${f.defGuns}× 76mm</span>`
        + `</div>`;
    // ── zone labels ──
    for (let r = 0; r < 5; r++) {
        const y = MAP_PAD_TOP + r * RS + Math.floor(HH / 2) - 6;
        const cls = r <= 1 ? "zone zone-def" : r >= 3 ? "zone zone-atk" : "zone zone-nml";
        html += `<div class="${cls}" style="top:${y}px">${ZONES[r]}</div>`;
    }
    // ── hexes ──
    for (let i = 0; i < N_TILES; i++) {
        const p = HP[i], t = tiles[i];
        const ah = f.bats.filter(b => b.side === "atk" && b.tile === i && !b.routed);
        const dh = f.bats.filter(b => b.side === "def" && b.tile === i && !b.routed);
        const contested = ah.length > 0 && dh.length > 0;
        html += `<div class="hex-wrap" style="left:${p.x}px;top:${p.y}px;width:${HW}px;height:${HH}px">`
            + `<div class="hex-bdr${contested ? " contested" : ""}" style="clip-path:${CLIP}"></div>`
            + `<div class="hex-fill ter-${t.terrain}" style="clip-path:${CLIP}"></div>`
            + `<div class="hex-info" style="clip-path:${CLIP}">`
            + `<span class="hex-id">${LABELS[i]}</span>`
            + `<span class="hex-ter">${esc(t.icon)} ${esc(t.terrain)} ×${t.mul}</span>`
            + `</div>`
            + `<div class="hex-units">${mkCounters(ah, dh)}</div>`
            + `</div>`;
    }
    // ── off-map attacker art (bottom) ──
    const botY = MAP_PAD_TOP + 4 * RS + HH + 8;
    html += `<div class="offmap offmap-atk" style="top:${botY}px">`
        + `<span class="offmap-lbl">OFF-MAP</span>`
        + `<span class="offmap-nm">394th Art Bn</span>`
        + `<span class="offmap-guns">${f.atkGuns}× 105mm</span>`
        + `</div>`;
    g.innerHTML = html;
}
function mkCounters(atks, defs) {
    return [...defs, ...atks].map((b, i) => {
        const side = b.side === "atk" ? "ctr-b" : "ctr-r";
        const fog = !b.revealed && b.side === "def" ? " ctr-fog" : "";
        const sup = b.suppression > 0 ? ` ctr-s${b.suppression}` : "";
        const ent = b.entrenched && !b.routed ? " ctr-ent" : "";
        const z = `z-index:${10 + i}`;
        if (fog)
            return `<div class="ctr ${side}${fog}" style="${z}">`
                + `<div class="ctr-hd"><span>II</span><span class="ctr-sym">╳</span><span>???</span></div>`
                + `<div class="ctr-nm">??? Bn</div>`
                + `<div class="ctr-row"><span>???</span><span>⚔???</span></div></div>`;
        const rn = b.rgt.replace("th", "");
        return `<div class="ctr ${side}${sup}${ent}" style="${z}" title="${b.id}  ${b.men}men  ⚔${fp(b)}FP  ${b.mg}MG ${b.mortar}Mor  M:${b.morale}  ${SUPP[b.suppression]}${b.entrenched ? "  DUG IN" : ""}">`
            + `<div class="ctr-hd"><span>II</span><span class="ctr-sym">╳</span><span>${esc(rn)}</span></div>`
            + `<div class="ctr-nm">${esc(b.bn)} Bn</div>`
            + `<div class="ctr-row"><span class="ctr-men">${b.men}</span><span class="ctr-fpv">⚔${fp(b)}</span></div>`
            + `<div class="ctr-row ctr-eq"><span>${b.mg}MG ${b.mortar}Mor</span><span>M:${b.morale}</span></div>`
            + (b.entrenched ? `<div class="ctr-dig">⚒ DUG IN</div>` : "")
            + `</div>`;
    }).join("");
}
// ── OOB tables ───────────────────────────────────────────
function renderOOB(fb) {
    $("oob-a").innerHTML = fb.filter(b => b.side === "atk").map(b => oobRow(b, true)).join("");
    $("oob-d").innerHTML = fb.filter(b => b.side === "def").map(b => oobRow(b, b.revealed || b.routed)).join("");
}
function oobRow(b, known) {
    if (!known)
        return `<tr class="oob-tr fog"><td class="oob-bn">${esc(b.bn)}</td><td colspan="8" class="oob-unk">???</td></tr>`;
    const cls = b.routed ? "oob-tr rt" : "oob-tr";
    const sup = b.suppression > 0 && !b.routed ? ` s${b.suppression}` : "";
    return `<tr class="${cls}${sup}">`
        + `<td class="oob-bn">${esc(b.bn)}</td>`
        + `<td class="oob-n">${b.routed ? "—" : b.men}</td>`
        + `<td class="oob-n">${b.mg}</td>`
        + `<td class="oob-n">${b.mortar}</td>`
        + `<td class="oob-n oob-fp">${fp(b)}</td>`
        + `<td class="oob-n">${b.morale}</td>`
        + `<td class="oob-s">${b.routed ? "ROUT" : SUPP[b.suppression]}</td>`
        + `<td class="oob-n">${b.entrenched ? "⚒" : ""}</td>`
        + `<td class="oob-h">${LABELS[b.tile]}</td>`
        + `</tr>`;
}
// ── Log ──────────────────────────────────────────────────
function renderLog(logEnd) {
    const el = $("log");
    el.innerHTML = "";
    const entries = fullLog.slice(0, logEnd);
    for (const e of entries.slice(-50)) {
        const d = document.createElement("div");
        d.className = "l-" + e.type;
        d.textContent = e.text;
        el.appendChild(d);
    }
    el.scrollTop = el.scrollHeight;
}
// ── Phase / controls ─────────────────────────────────────
function renderPhaseBar(f) {
    $("phase").textContent = `TURN ${f.turn}  ·  ${PH[f.phase]}`;
    $("step").textContent = `${stepIdx + 1} / ${steps.length}`;
    $("progress").style.width = `${(stepIdx / Math.max(1, steps.length - 1)) * 100}%`;
}
function renderControls() {
    $("prev").disabled = stepIdx <= 0;
    $("next").disabled = stepIdx >= steps.length - 1;
    $("skip").disabled = stepIdx >= steps.length - 1;
}
// ── Results ──────────────────────────────────────────────
function renderResults() {
    const ini = steps[0].bats;
    const fin = steps[steps.length - 1].bats;
    let verdict = "";
    for (let i = fullLog.length - 1; i >= 0; i--) {
        if (fullLog[i].type === "result" && fullLog[i].text.includes("★")) {
            verdict = fullLog[i].text;
            break;
        }
    }
    let h = `<div class="res-v">${esc(verdict)}</div>`;
    h += `<div class="res-sub">Turns: ${steps[steps.length - 1].turn}  │  Atk Art: ${steps[steps.length - 1].atkGuns}/${steps[0].atkGuns} guns  │  Def Art: ${steps[steps.length - 1].defGuns}/${steps[0].defGuns} guns</div>`;
    for (const side of ["atk", "def"]) {
        const label = side === "atk" ? "394th Infantry Regiment" : "1028th Rifle Regiment";
        h += `<div class="res-rgt">${esc(label)}</div>`;
        h += `<table class="res-t"><thead><tr><th>Bn</th><th>Men</th><th></th><th>End</th><th>Lost</th><th>MG</th><th>Mor</th><th>⚔</th></tr></thead><tbody>`;
        for (const ib of ini.filter(b => b.side === side)) {
            const fb = fin.find(b => b.id === ib.id);
            const lost = ib.men - Math.max(0, fb.men);
            const r = fb.routed ? ' class="rt"' : '';
            h += `<tr${r}><td>${esc(ib.bn)}</td><td>${ib.men}</td><td>→</td>`
                + `<td>${fb.routed ? "0" : Math.max(0, fb.men)}</td>`
                + `<td class="res-lost">−${lost}</td>`
                + `<td>${ib.mg}→${fb.mg}</td><td>${ib.mortar}→${fb.mortar}</td>`
                + `<td>${fp(ib)}→${fp(fb)}</td></tr>`;
        }
        const s0 = ini.filter(b => b.side === side).reduce((s, b) => s + b.men, 0);
        const s1 = fin.filter(b => b.side === side).reduce((s, b) => b.routed ? s : s + Math.max(0, b.men), 0);
        const fp0 = ini.filter(b => b.side === side).reduce((s, b) => s + fp(b), 0);
        const fp1 = fin.filter(b => b.side === side).reduce((s, b) => b.routed ? s : s + fp(b), 0);
        h += `<tr class="res-tot"><td>Tot</td><td>${s0}</td><td>→</td><td>${s1}</td><td class="res-lost">−${s0 - s1}</td><td></td><td></td><td>${fp0}→${fp1}</td></tr>`;
        h += `</tbody></table>`;
    }
    $("results").innerHTML = h;
}
// ── Wire up ──────────────────────────────────────────────
$("prev").addEventListener("click", () => { if (stepIdx > 0) {
    stepIdx--;
    renderFrame();
} });
$("next").addEventListener("click", () => { if (stepIdx < steps.length - 1) {
    stepIdx++;
    renderFrame();
} });
$("skip").addEventListener("click", () => { stepIdx = steps.length - 1; renderFrame(); });
$("reset").addEventListener("click", initBattle);
initBattle();
