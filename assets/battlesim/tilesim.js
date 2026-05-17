"use strict";
// =================================================================
//  BATTLES -- Regiment Combat on a 5-deep Hex Grid
// =================================================================
//  milsymbol NATO counters, recon detachments, off-map artillery,
//  flanking fire, entrenchment.  No emojis -- text only.
// =================================================================
// -- Grid constants -----------------------------------------------
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
const TERRAIN_DATA = [
    { name: "field", mul: 1.0 },
    { name: "hill", mul: 1.5 },
    { name: "village", mul: 1.8 },
    { name: "forest", mul: 2.0 },
    { name: "ridge", mul: 1.7 },
];
const SUPP = ["READY", "DISRUPTED", "SUPPRESSED", "PINNED"];
const PH = ["DEPLOY", "RECON", "ARTILLERY", "ASSAULT", "MOVEMENT"];
// SIDC codes (15-char, MIL-STD-2525C)
const SIDC_ATK_INF = "SFGPUCI---F---"; // friendly infantry bn
const SIDC_DEF_INF = "SHGPUCI---F---"; // hostile infantry bn
const SIDC_ATK_RCN = "SFGPUCR---D---"; // friendly recon plt
const SIDC_ATK_ART = "SFGPUCF---F---"; // friendly art bn
const SIDC_DEF_ART = "SHGPUCF---E---"; // hostile art btry
const SIDC_UNK = "SHGPUCI---F---"; // unknown hostile (shown foggy)
// hex layout
const HW = 110, HH = 127;
const CS = HW + 4, RS = Math.floor(HH * .75) + 4, HO = Math.floor(CS / 2);
const CLIP = "polygon(50% 0%,100% 25%,100% 75%,50% 100%,0% 75%,0% 25%)";
const MAP_PAD_TOP = 40;
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
// -- State --------------------------------------------------------
let tiles = [];
let units = [];
let fullLog = [];
let turn = 0, phase = 0, over = false;
let atkGuns = 12, defGuns = 8;
let steps = [];
let stepIdx = 0;
// -- Helpers ------------------------------------------------------
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
function cv(u) {
    return Math.round(u.rifles / 10 + u.mg + u.mortar * 0.67);
}
function applyCas(u, n) {
    const actual = Math.min(n, Math.max(0, u.men));
    u.men -= actual;
    u.rifles = Math.max(0, u.rifles - Math.floor(actual * 0.85));
    if (actual >= 25)
        u.mg = Math.max(0, u.mg - Math.floor(actual / 70));
    if (actual >= 50 && u.mortar > 0 && d6() <= 2)
        u.mortar--;
}
function snap() {
    return { turn, phase, units: units.map(u => ({ ...u })), atkGuns, defGuns, logEnd: fullLog.length, over };
}
// milsymbol SVG cache
const _svgCache = new Map();
function symSvg(sidc, size) {
    const key = sidc + ":" + size;
    let s = _svgCache.get(key);
    if (!s) {
        if (typeof ms !== "undefined") {
            s = new ms.Symbol(sidc, { size }).asSVG();
        }
        else {
            s = `<span style="font-size:${size}px;color:#888">[${sidc.substring(4, 7)}]</span>`;
        }
        _svgCache.set(key, s);
    }
    return s;
}
function detachRecon(parent) {
    const men = 35;
    const rif = 30;
    parent.men -= men;
    parent.rifles -= rif;
    parent.mg = Math.max(0, parent.mg - 1);
    return {
        id: parent.id + "/R",
        name: parent.name + "/R",
        rgt: parent.rgt,
        side: parent.side,
        type: "recon",
        size: "plt",
        men,
        rifles: rif,
        mg: 1,
        mortar: 0,
        morale: Math.min(100, parent.morale + 5),
        suppression: 0,
        tile: parent.tile,
        revealed: parent.revealed,
        routed: false,
        entrenched: false,
        sidc: parent.side === "atk" ? SIDC_ATK_RCN : SIDC_ATK_RCN,
        parent: parent.id,
    };
}
// -- Init & Simulate ----------------------------------------------
function initBattle() {
    _svgCache.clear();
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
        const info = TERRAIN_DATA.find(x => x.name === t);
        return { index: i, terrain: t, mul: info.mul };
    });
    units = [
        mkBn("I", "394th", "atk", 820, 700, 12, 6, 85, 15),
        mkBn("II", "394th", "atk", 790, 670, 12, 6, 80, 15),
        mkBn("III", "394th", "atk", 850, 740, 14, 8, 90, 15),
        mkBn("I", "1028th", "def", 680, 580, 8, 4, 70, 3),
        mkBn("II", "1028th", "def", 720, 620, 10, 6, 75, 5),
        mkBn("III", "1028th", "def", 650, 560, 8, 4, 65, 6),
    ];
    // one defender forward as outpost
    const defIdx = rand(3);
    units[3 + defIdx].tile = [7, 8, 9][rand(3)];
    units[3 + defIdx].entrenched = false;
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
function mkBn(name, rgt, side, men, rifles, mg, mortar, morale, tile) {
    return {
        id: name + "/" + rgt.replace("th", ""),
        name, rgt, side, type: "inf", size: "bn",
        men, rifles, mg, mortar, morale,
        suppression: 0, tile, revealed: side === "atk",
        routed: false, entrenched: side === "def",
        sidc: side === "atk" ? SIDC_ATK_INF : SIDC_DEF_INF,
    };
}
function simulateAll() {
    steps = [];
    const atk = units.filter(u => u.side === "atk" && u.type === "inf");
    const def = units.filter(u => u.side === "def");
    addLog("== 394th Infantry Rgt  vs  1028th Rifle Rgt ==", "hdr");
    addLog(`394th: ${atk.reduce((s, u) => s + u.men, 0)} men  CV:${atk.reduce((s, u) => s + cv(u), 0)}  |  Off-map: ${atkGuns}x 105mm`, "info");
    addLog(`1028th: ${def.reduce((s, u) => s + u.men, 0)} men  CV:${def.reduce((s, u) => s + cv(u), 0)}  |  Off-map: ${defGuns}x 76mm  |  ENTRENCHED`, "info");
    addLog("Objective: breach the main line and reach the A-row.", "info");
    steps.push(snap());
    while (!over) {
        phase++;
        if (phase > 4) {
            phase = 1;
            turn++;
            for (const u of units)
                if (!u.routed)
                    u.suppression = Math.max(0, u.suppression - 1);
            addLog(`\n==== TURN ${turn} ====`, "hdr");
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
// -- 1. RECON -----------------------------------------------------
function doRecon() {
    addLog("-- Recon  0500-0700 --", "hdr");
    // Turn 1: detach recon platoons from attacker battalions
    if (turn === 1) {
        const bns = units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed);
        for (const bn of bns) {
            const r = detachRecon(bn);
            units.push(r);
            addLog(`${r.id} detaches from ${bn.id} (${r.men} men, ${r.mg} MG)`, "recon");
        }
    }
    // Advance recon platoons forward
    const recons = units.filter(u => u.type === "recon" && u.side === "atk" && !u.routed);
    for (const r of recons) {
        // withdraw if on tile with enemy
        if (units.some(u => u.side === "def" && u.tile === r.tile && !u.routed)) {
            const back = ADJ[r.tile].filter(t => ROW[t] > ROW[r.tile]);
            if (back.length) {
                const from = r.tile;
                r.tile = back[rand(back.length)];
                addLog(`${r.id} withdraws from contact: ${LABELS[from]} -> ${LABELS[r.tile]}`, "recon");
            }
            continue;
        }
        // advance: prefer forward + empty
        const fw = ADJ[r.tile]
            .filter(t => ROW[t] < ROW[r.tile])
            .sort((a, b) => {
            const oa = units.filter(u => u.tile === a && !u.routed).length;
            const ob = units.filter(u => u.tile === b && !u.routed).length;
            return oa - ob;
        });
        if (fw.length) {
            const from = r.tile;
            r.tile = fw[0];
            addLog(`${r.id} scouts ${LABELS[from]} -> ${LABELS[r.tile]}`, "recon");
        }
    }
    // Recon platoons auto-spot their current tile
    for (const r of units.filter(u => u.type === "recon" && u.side === "atk" && !u.routed)) {
        for (const d of units.filter(u => u.side === "def" && u.tile === r.tile && !u.routed && !u.revealed)) {
            d.revealed = true;
            addLog(`${r.id} discovers ${d.id} at ${LABELS[r.tile]}!`, "recon");
            if (d6() <= 3) {
                const cas = rand(3, 8);
                applyCas(r, cas);
                addLog(`  ${r.id} takes fire: -${cas} men`, "combat");
                if (r.men <= 5) {
                    r.routed = true;
                    addLog(`  ${r.id} destroyed`, "result");
                }
            }
        }
    }
    // All units spot adjacent tiles
    for (const a of units.filter(u => u.side === "atk" && !u.routed)) {
        const bonus = a.type === "recon" ? 2 : 0;
        for (const ti of ADJ[a.tile]) {
            const hidden = units.filter(u => u.side === "def" && u.tile === ti && !u.routed && !u.revealed);
            for (const d of hidden) {
                const roll = d6() + bonus;
                const pen = tiles[ti].terrain === "forest" ? 2 : tiles[ti].terrain === "village" ? 1 : 0;
                if (roll - pen >= 4) {
                    d.revealed = true;
                    const ent = d.entrenched ? " [ENTRENCHED]" : "";
                    addLog(`${a.id} -> ${LABELS[ti]}: spotted ${d.id} (~${Math.round(d.men / 50) * 50} men, ${d.mg}MG)${ent}`, "recon");
                }
                else if (roll === 1) {
                    const cas = rand(3, 12);
                    applyCas(a, cas);
                    addLog(`${a.id} ambushed near ${LABELS[ti]}: -${cas} men`, "combat");
                    if (a.type === "recon" && a.men <= 5) {
                        a.routed = true;
                        addLog(`  ${a.id} destroyed`, "result");
                    }
                }
            }
        }
    }
}
// -- 2. ARTILLERY (off-map) ---------------------------------------
function doArtillery() {
    addLog("-- Artillery  0700-0900 --", "hdr");
    // ATTACKER: 394th Art Bn
    if (atkGuns > 0) {
        const tgt = new Set();
        for (const u of units)
            if (u.side === "def" && u.revealed && !u.routed)
                tgt.add(u.tile);
        if (tgt.size === 0) {
            addLog(`394th Art Bn (${atkGuns}x 105mm): no targets spotted`, "arty");
        }
        else {
            const targets = [...tgt];
            const perTgt = Math.ceil(atkGuns / targets.length);
            let used = 0;
            for (const ti of targets) {
                const guns = Math.min(perTgt, atkGuns - used);
                used += guns;
                for (const d of units.filter(u => u.side === "def" && u.tile === ti && !u.routed)) {
                    let cas = 0;
                    for (let g = 0; g < guns; g++)
                        cas += Math.floor(rand(3, 8) / tiles[ti].mul);
                    applyCas(d, cas);
                    const sl = guns >= 7 ? 3 : guns >= 4 ? 2 : guns >= 2 ? 1 : 0;
                    d.suppression = Math.max(d.suppression, sl);
                    d.morale = clamp(d.morale - rand(3, 7), 0, 100);
                    addLog(`394th Art (${guns}x 105mm) -> ${LABELS[ti]}: ${d.id} -${cas} men, ${SUPP[d.suppression]}`, "arty");
                }
            }
        }
    }
    // DEFENDER: 1028th Art Btry
    if (defGuns > 0) {
        const atkTiles = [];
        for (const u of units)
            if (u.side === "atk" && u.type === "inf" && !u.routed && !atkTiles.includes(u.tile))
                atkTiles.push(u.tile);
        if (atkTiles.length > 0) {
            const ti = atkTiles[rand(atkTiles.length)];
            const tgts = units.filter(u => u.side === "atk" && u.tile === ti && !u.routed);
            for (const a of tgts) {
                const cas = Math.max(1, Math.floor(rand(2, 5) * defGuns / Math.max(1, tgts.length) / tiles[ti].mul));
                applyCas(a, cas);
                addLog(`1028th Art (${defGuns}x 76mm) -> ${LABELS[ti]}: ${a.id} -${cas} men`, "arty");
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
// -- 3. ASSAULT ---------------------------------------------------
function doAssault() {
    addLog("-- Assault  0900-1100 --", "hdr");
    // Recon platoons withdraw from contested tiles before assault
    for (const r of units.filter(u => u.type === "recon" && !u.routed)) {
        if (units.some(u => u.side !== r.side && u.tile === r.tile && !u.routed)) {
            const back = ADJ[r.tile].filter(t => r.side === "atk" ? ROW[t] > ROW[r.tile] : ROW[t] < ROW[r.tile]);
            if (back.length) {
                const from = r.tile;
                r.tile = back[rand(back.length)];
                addLog(`${r.id} withdraws before assault: ${LABELS[from]} -> ${LABELS[r.tile]}`, "move");
            }
        }
    }
    const plan = new Map();
    for (const a of units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed)) {
        const targets = ADJ[a.tile]
            .filter(t => ROW[t] < ROW[a.tile])
            .filter(t => units.some(u => u.side === "def" && u.tile === t && !u.routed && u.revealed));
        if (units.some(u => u.side === "def" && u.tile === a.tile && !u.routed))
            targets.push(a.tile);
        if (!targets.length)
            continue;
        const best = targets.reduce((a2, b2) => {
            const s = (ti) => units.filter(u => u.side === "def" && u.tile === ti && !u.routed).reduce((x, u) => x + cv(u), 0);
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
        const ds = units.filter(u => u.side === "def" && u.tile === ti && !u.routed);
        if (ds.length > 0)
            resolveCombat(atks, ds, ti, attackedTiles);
    }
}
function resolveCombat(atks, defs, ti, attackedTiles) {
    const tile = tiles[ti];
    const atkCV = atks.reduce((s, a) => s + cv(a), 0);
    const defCV = defs.reduce((s, d) => s + cv(d), 0);
    if (atkCV <= 0 || defCV <= 0)
        return;
    // flanking fire from adjacent un-attacked defenders
    let flankCV = 0;
    const flankFrom = [];
    for (const adj of ADJ[ti]) {
        if (attackedTiles.has(adj))
            continue;
        for (const fd of units.filter(u => u.side === "def" && u.tile === adj && !u.routed)) {
            const contrib = Math.floor(cv(fd) * 0.3);
            if (contrib > 0) {
                flankCV += contrib;
                flankFrom.push(LABELS[adj]);
            }
        }
    }
    const totalDefCV = defCV + flankCV;
    if (flankCV > 0)
        addLog(`  flanking fire from ${[...new Set(flankFrom)].join(",")}: +CV:${flankCV}`, "combat");
    // entrenchment
    const entMul = defs.some(d => d.entrenched) ? 1.3 : 1.0;
    if (entMul > 1)
        addLog(`  defenders entrenched: x${entMul} bonus`, "combat");
    const tAmen = atks.reduce((s, a) => s + a.men, 0);
    const rawRatio = atkCV / totalDefCV;
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
            addLog(`  X ${a.id} ROUTS (${a.men} men)`, "result");
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
            addLog(`  X ${d.id} ROUTS (${d.men} flee)`, "result");
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
    addLog(`${atks.map(a => a.id).join("+")} -> ${LABELS[ti]}: ${out}  [CV ${rawRatio.toFixed(1)}:1 ter x${(tile.mul * entMul).toFixed(1)} -> ${eff.toFixed(1)} eff]  A-${aLoss} D-${dLoss}`, "combat");
}
// -- 4. MOVEMENT --------------------------------------------------
function doMovement() {
    addLog("-- Movement  1100-1200 --", "hdr");
    let moved = false;
    // Recon platoons advance
    for (const r of units.filter(u => u.type === "recon" && u.side === "atk" && !u.routed)) {
        if (units.some(u => u.side === "def" && u.tile === r.tile && !u.routed))
            continue;
        const fw = ADJ[r.tile]
            .filter(t => ROW[t] < ROW[r.tile])
            .sort((a, b) => {
            const oa = units.filter(u => u.tile === a && !u.routed).length;
            const ob = units.filter(u => u.tile === b && !u.routed).length;
            return oa - ob;
        });
        if (fw.length) {
            const from = r.tile;
            r.tile = fw[0];
            addLog(`${r.id} scouts ahead: ${LABELS[from]} -> ${LABELS[r.tile]}`, "move");
            moved = true;
        }
    }
    // Infantry battalions advance
    for (const a of units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed)) {
        if (units.some(u => u.side === "def" && u.tile === a.tile && !u.routed))
            continue;
        const fw = ADJ[a.tile].filter(t => ROW[t] < ROW[a.tile]);
        if (!fw.length)
            continue;
        fw.sort((x, y) => {
            const dx = units.filter(u => u.side === "def" && u.tile === x && !u.routed).length;
            const dy = units.filter(u => u.side === "def" && u.tile === y && !u.routed).length;
            if (dx !== dy)
                return dx - dy;
            const fx = units.filter(u => u.side === "atk" && u.type === "inf" && u.tile === x && !u.routed).length;
            const fy = units.filter(u => u.side === "atk" && u.type === "inf" && u.tile === y && !u.routed).length;
            return fx - fy;
        });
        const from = a.tile;
        a.tile = fw[0];
        addLog(`${a.id} advances ${LABELS[from]} -> ${LABELS[a.tile]}`, "move");
        moved = true;
    }
    // Defender retreat
    for (const d of units.filter(u => u.side === "def" && !u.routed && u.morale < 35)) {
        const bk = ADJ[d.tile].filter(t => ROW[t] < ROW[d.tile]);
        if (!bk.length)
            continue;
        const from = d.tile;
        d.tile = bk[rand(bk.length)];
        d.entrenched = false;
        addLog(`${d.id} retreats ${LABELS[from]} -> ${LABELS[d.tile]} (M:${d.morale}, lost entrenchment)`, "move");
        moved = true;
    }
    if (!moved)
        addLog("No movement.", "info");
}
// -- Victory ------------------------------------------------------
function checkEnd() {
    const breach = units.filter(u => u.side === "atk" && !u.routed && u.type === "inf" && ROW[u.tile] === 0);
    const defAlive = units.filter(u => u.side === "def" && !u.routed);
    const atkAlive = units.filter(u => u.side === "atk" && !u.routed && u.type === "inf");
    if (breach.length) {
        over = true;
        addLog(`\n* ATTACKER VICTORY -- ${breach[0].id} reached ${LABELS[breach[0].tile]}`, "result");
    }
    else if (!defAlive.length) {
        over = true;
        addLog("\n* ATTACKER VICTORY -- all defenders routed", "result");
    }
    else if (!atkAlive.length) {
        over = true;
        addLog("\n* DEFENDER VICTORY -- all attackers routed", "result");
    }
    else if (turn >= 10) {
        over = true;
        addLog(`\n* STALEMATE -- ${turn} turns`, "result");
    }
}
// =================================================================
//  RENDERING
// =================================================================
function renderFrame() {
    const f = steps[stepIdx];
    renderGrid(f);
    renderOOB(f.units);
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
// -- Hex grid + off-map -------------------------------------------
function renderGrid(f) {
    const g = $("hex-grid");
    let html = "";
    // off-map defender art (top)
    html += `<div class="offmap offmap-def">`
        + `<div class="offmap-sym">${symSvg(SIDC_DEF_ART, 28)}</div>`
        + `<div class="offmap-info"><div class="offmap-nm">1028th Art Btry</div>`
        + `<div class="offmap-det">${f.defGuns}x 76mm | ${f.defGuns > 0 ? "READY" : "DESTROYED"}</div></div></div>`;
    // zone labels
    for (let r = 0; r < 5; r++) {
        const y = MAP_PAD_TOP + r * RS + Math.floor(HH / 2) - 6;
        const cls = r <= 1 ? "zone zone-def" : r >= 3 ? "zone zone-atk" : "zone zone-nml";
        html += `<div class="${cls}" style="top:${y}px">${ZONES[r]}</div>`;
    }
    // hexes
    for (let i = 0; i < N_TILES; i++) {
        const p = HP[i], t = tiles[i];
        const onTile = f.units.filter(u => u.tile === i && !u.routed);
        const hasAtk = onTile.some(u => u.side === "atk");
        const hasDef = onTile.some(u => u.side === "def");
        const contested = hasAtk && hasDef;
        html += `<div class="hex-wrap" style="left:${p.x}px;top:${p.y}px;width:${HW}px;height:${HH}px">`
            + `<div class="hex-bdr${contested ? " contested" : ""}" style="clip-path:${CLIP}"></div>`
            + `<div class="hex-fill ter-${t.terrain}" style="clip-path:${CLIP}"></div>`
            + `<div class="hex-info" style="clip-path:${CLIP}">`
            + `<span class="hex-id">${LABELS[i]}</span>`
            + `<span class="hex-ter">${esc(t.terrain)} x${t.mul}</span>`
            + `</div>`
            + `<div class="hex-units">${mkUnits(onTile)}</div>`
            + `</div>`;
    }
    // off-map attacker art (bottom)
    const botY = MAP_PAD_TOP + 4 * RS + HH + 8;
    html += `<div class="offmap offmap-atk" style="top:${botY}px">`
        + `<div class="offmap-sym">${symSvg(SIDC_ATK_ART, 28)}</div>`
        + `<div class="offmap-info"><div class="offmap-nm">394th Art Bn</div>`
        + `<div class="offmap-det">${f.atkGuns}x 105mm | ${f.atkGuns > 0 ? "READY" : "DESTROYED"}</div></div></div>`;
    g.innerHTML = html;
}
function mkUnits(on) {
    // sort: defenders first, then attackers; infantry before recon
    const sorted = [...on].sort((a, b) => {
        if (a.side !== b.side)
            return a.side === "def" ? -1 : 1;
        if (a.type !== b.type)
            return a.type === "inf" ? -1 : 1;
        return 0;
    });
    return sorted.map((u, i) => mkUnit(u, 10 + i)).join("");
}
function mkUnit(u, z) {
    const fog = !u.revealed && u.side === "def";
    const sup = u.suppression > 0 ? ` u-s${u.suppression}` : "";
    const cls = u.side === "atk" ? "u-atk" : "u-def";
    const szPx = u.size === "plt" ? 22 : 30;
    if (fog) {
        return `<div class="unit ${cls} u-fog" style="z-index:${z}" title="Unidentified enemy unit">`
            + `<div class="u-sym">${symSvg(SIDC_UNK, 24)}</div>`
            + `<div class="u-label">???</div></div>`;
    }
    const ent = u.entrenched ? " ENT" : "";
    const st = u.suppression > 0 ? " " + SUPP[u.suppression].substring(0, 4) : "";
    const tip = `${u.id}  ${u.men} men  CV:${cv(u)}  ${u.mg}MG ${u.mortar}Mor  M:${u.morale}  ${SUPP[u.suppression]}${u.entrenched ? "  ENTRENCHED" : ""}`;
    const typeLabel = u.type === "recon" ? "RCN" : "";
    return `<div class="unit ${cls}${sup}" style="z-index:${z}" title="${esc(tip)}">`
        + `<div class="u-sym">${symSvg(u.sidc, szPx)}</div>`
        + `<div class="u-label">${u.men} CV:${cv(u)}${ent}${st}</div>`
        + (typeLabel ? `<div class="u-type">${typeLabel}</div>` : "")
        + `</div>`;
}
// -- OOB tables ---------------------------------------------------
function renderOOB(fu) {
    // Attacker: interleave parent bn with its recon sub-unit
    let aHtml = "";
    for (const bn of fu.filter(u => u.side === "atk" && u.type === "inf")) {
        aHtml += oobRow(bn, true);
        const rcn = fu.find(u => u.parent === bn.id);
        if (rcn)
            aHtml += oobRow(rcn, true);
    }
    $("oob-a").innerHTML = aHtml;
    // Defender
    $("oob-d").innerHTML = fu.filter(u => u.side === "def").map(u => oobRow(u, u.revealed || u.routed)).join("");
}
function oobRow(u, known) {
    const indent = u.parent ? "oob-sub" : "";
    if (!known)
        return `<tr class="oob-tr fog"><td class="oob-bn ${indent}">${esc(u.name)}</td><td colspan="9" class="oob-unk">???</td></tr>`;
    const cls = u.routed ? "oob-tr rt" : "oob-tr";
    const sup = u.suppression > 0 && !u.routed ? ` s${u.suppression}` : "";
    const typeStr = u.type === "recon" ? "RCN" : "INF";
    return `<tr class="${cls}${sup}">`
        + `<td class="oob-bn ${indent}">${esc(u.name)}</td>`
        + `<td class="oob-ty">${typeStr}</td>`
        + `<td class="oob-n">${u.routed ? "--" : u.men}</td>`
        + `<td class="oob-n">${u.mg}</td>`
        + `<td class="oob-n">${u.mortar}</td>`
        + `<td class="oob-n oob-cv">${cv(u)}</td>`
        + `<td class="oob-n">${u.morale}</td>`
        + `<td class="oob-s">${u.routed ? "ROUT" : SUPP[u.suppression]}</td>`
        + `<td class="oob-n">${u.entrenched ? "ENT" : ""}</td>`
        + `<td class="oob-h">${LABELS[u.tile]}</td>`
        + `</tr>`;
}
// -- Log ----------------------------------------------------------
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
// -- Phase / controls ---------------------------------------------
function renderPhaseBar(f) {
    $("phase").textContent = `TURN ${f.turn}  |  ${PH[f.phase]}`;
    $("step").textContent = `${stepIdx + 1} / ${steps.length}`;
    $("progress").style.width = `${(stepIdx / Math.max(1, steps.length - 1)) * 100}%`;
}
function renderControls() {
    $("prev").disabled = stepIdx <= 0;
    $("next").disabled = stepIdx >= steps.length - 1;
    $("skip").disabled = stepIdx >= steps.length - 1;
}
// -- Results ------------------------------------------------------
function renderResults() {
    const ini = steps[0].units;
    const fin = steps[steps.length - 1].units;
    let verdict = "";
    for (let i = fullLog.length - 1; i >= 0; i--) {
        if (fullLog[i].type === "result" && fullLog[i].text.includes("*")) {
            verdict = fullLog[i].text;
            break;
        }
    }
    let h = `<div class="res-v">${esc(verdict)}</div>`;
    h += `<div class="res-sub">Turns: ${steps[steps.length - 1].turn}  |  Atk Art: ${steps[steps.length - 1].atkGuns}/${steps[0].atkGuns} guns  |  Def Art: ${steps[steps.length - 1].defGuns}/${steps[0].defGuns} guns</div>`;
    for (const side of ["atk", "def"]) {
        const label = side === "atk" ? "394th Infantry Regiment" : "1028th Rifle Regiment";
        h += `<div class="res-rgt">${esc(label)}</div>`;
        h += `<table class="res-t"><thead><tr><th>Unit</th><th>Type</th><th>Men</th><th></th><th>End</th><th>Lost</th><th>MG</th><th>Mor</th><th>CV</th></tr></thead><tbody>`;
        const sideUnits = ini.filter(u => u.side === side);
        for (const iu of sideUnits) {
            const fu = fin.find(u => u.id === iu.id);
            if (!fu)
                continue;
            const lost = iu.men - Math.max(0, fu.men);
            const r = fu.routed ? ' class="rt"' : '';
            const typeStr = iu.type === "recon" ? "RCN" : "INF";
            h += `<tr${r}><td>${esc(iu.name)}</td><td>${typeStr}</td><td>${iu.men}</td><td>-></td>`
                + `<td>${fu.routed ? "0" : Math.max(0, fu.men)}</td>`
                + `<td class="res-lost">-${lost}</td>`
                + `<td>${iu.mg}->${fu.mg}</td><td>${iu.mortar}->${fu.mortar}</td>`
                + `<td>${cv(iu)}->${cv(fu)}</td></tr>`;
        }
        const s0 = sideUnits.reduce((s, u) => s + u.men, 0);
        const s1 = fin.filter(u => u.side === side).reduce((s, u) => u.routed ? s : s + Math.max(0, u.men), 0);
        const cv0 = sideUnits.reduce((s, u) => s + cv(u), 0);
        const cv1 = fin.filter(u => u.side === side).reduce((s, u) => u.routed ? s : s + cv(u), 0);
        h += `<tr class="res-tot"><td>Total</td><td></td><td>${s0}</td><td>-></td><td>${s1}</td><td class="res-lost">-${s0 - s1}</td><td></td><td></td><td>${cv0}->${cv1}</td></tr>`;
        h += `</tbody></table>`;
    }
    $("results").innerHTML = h;
}
// -- Wire up ------------------------------------------------------
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
