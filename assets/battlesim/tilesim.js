"use strict";
// =================================================================
//  BATTLES -- Regiment Combat on a 5-deep Hex Grid
// =================================================================
//  milsymbol NATO counters, recon detachments, off-map artillery,
//  flanking fire, entrenchment.  No emojis -- text only.
// =================================================================
// -- Grid constants -----------------------------------------------
const LABELS = [
    "R1", "R2", "R3", "R4",
    "A1", "A2", "A3",
    "B1", "B2", "B3", "B4",
    "C1", "C2", "C3",
    "D1", "D2", "D3", "D4",
    "E1", "E2", "E3",
    "F1", "F2", "F3", "F4",
];
const N_TILES = 25;
const ADJ = [
    /*  0 R1 */ [1, 4],
    /*  1 R2 */ [0, 2, 4, 5],
    /*  2 R3 */ [1, 3, 5, 6],
    /*  3 R4 */ [2, 6],
    /*  4 A1 */ [0, 1, 5, 7, 8],
    /*  5 A2 */ [1, 2, 4, 6, 8, 9],
    /*  6 A3 */ [2, 3, 5, 9, 10],
    /*  7 B1 */ [4, 8, 11],
    /*  8 B2 */ [4, 5, 7, 9, 11, 12],
    /*  9 B3 */ [5, 6, 8, 10, 12, 13],
    /* 10 B4 */ [6, 9, 13],
    /* 11 C1 */ [7, 8, 12, 14, 15],
    /* 12 C2 */ [8, 9, 11, 13, 15, 16],
    /* 13 C3 */ [9, 10, 12, 16, 17],
    /* 14 D1 */ [11, 15, 18],
    /* 15 D2 */ [11, 12, 14, 16, 18, 19],
    /* 16 D3 */ [12, 13, 15, 17, 19, 20],
    /* 17 D4 */ [13, 16, 20],
    /* 18 E1 */ [14, 15, 19, 21, 22],
    /* 19 E2 */ [15, 16, 18, 20, 22, 23],
    /* 20 E3 */ [16, 17, 19, 23, 24],
    /* 21 F1 */ [18, 22],
    /* 22 F2 */ [18, 19, 21, 23],
    /* 23 F3 */ [19, 20, 22, 24],
    /* 24 F4 */ [20, 23],
];
const ROW = [0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6];
const ZONES = ["DEFENDER EXIT", "DEFENDER REAR", "MAIN LINE", "NO MAN'S LAND", "APPROACH", "FORWARD POS", "ATTACKER START"];
const TERRAIN_DATA = [
    { name: "field", mul: 1.0 },
    { name: "hill", mul: 1.5 },
    { name: "village", mul: 1.8 },
    { name: "forest", mul: 2.0 },
    { name: "ridge", mul: 1.7 },
];
const SUPP = ["READY", "DISRUPTED", "SUPPRESSED", "PINNED"];
// SIDC codes (15-char, MIL-STD-2525C)
const SIDC_ATK_INF = "SFGPUCI----F---"; // friendly infantry bn
const SIDC_DEF_INF = "SHGPUCI----F---"; // hostile infantry bn
const SIDC_ATK_RCN = "SFGPUCR----D---"; // friendly recon plt
const SIDC_ATK_ART = "SFGPUCF----F---"; // friendly art bn
const SIDC_DEF_ART = "SHGPUCF----E---"; // hostile art btry
const SIDC_UNK = "SHGPUCI----F---"; // unknown hostile (shown foggy)
// hex layout
const HW = 104, HH = 120;
const CS = HW + 4, RS = Math.floor(HH * .75) + 4, HO = Math.floor(CS / 2);
const CLIP = "polygon(50% 0%,100% 25%,100% 75%,50% 100%,0% 75%,0% 25%)";
const MAP_PAD_TOP = 40;
const HP = [];
{
    const rows = [[0, 1, 2, 3], [4, 5, 6], [7, 8, 9, 10], [11, 12, 13], [14, 15, 16, 17], [18, 19, 20], [21, 22, 23, 24]];
    for (let r = 0; r < 7; r++) {
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
let turn = 0, over = false;
let atkGuns = 12, defGuns = 8;
let steps = [];
let stepIdx = 0;
let scoutingBns = new Set(); // bn IDs doing recon this turn
let scoutCount = new Map(); // consecutive recon turns per bn
let atkSupport = { hmg: 8, hmortar: 4, at: 4 };
let supportTarget = ""; // bn ID receiving support this hour
// -- Helpers ------------------------------------------------------
function rand(a, b) {
    if (b === undefined)
        return Math.floor(Math.random() * a);
    return a + Math.floor(Math.random() * (b - a + 1));
}
function d6() { return rand(1, 6); }
function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }
function hourStr(t) { return String(t + 5).padStart(2, '0') + '00'; }
function $(id) { return document.getElementById(id); }
function esc(s) { const el = document.createElement("span"); el.textContent = s; return el.innerHTML; }
function addLog(t, ty = "info") { fullLog.push({ text: t, type: ty }); }
function cv(u) {
    return Math.round(u.rifles / 10 + u.mg + u.mortar * 0.67 + u.at * 1.5);
}
function applyCas(u, n) {
    const actual = Math.min(n, Math.max(0, u.men));
    u.men -= actual;
    u.rifles = Math.max(0, u.rifles - Math.floor(actual * 0.85));
    if (actual >= 25)
        u.mg = Math.max(0, u.mg - Math.floor(actual / 70));
    if (actual >= 50 && u.mortar > 0 && d6() <= 2)
        u.mortar--;
    if (actual >= 40 && u.at > 0 && d6() <= 2)
        u.at--;
}
function snap(reconOut = false) {
    return { turn, reconOut, units: units.map(u => ({ ...u })), atkGuns, defGuns, atkSupport: { ...atkSupport }, logEnd: fullLog.length, over };
}
// milsymbol SVG cache
const _svgCache = new Map();
function symSvg(sidc, size) {
    const key = sidc + ":" + size;
    let s = _svgCache.get(key);
    if (!s) {
        if (typeof ms !== "undefined") {
            s = new ms.Symbol(sidc, { size, frame: true, fill: true, strokeWidth: 3 }).asSVG();
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
        at: 0,
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
        "field", "field", "field", "field", // R-row (exit)
        "field", "field", "field", // A-row (rear)
        "hill", "village", "forest", "ridge", // B-row (main line - strong terrain)
        "field", "field", "field", // C-row (no man's land)
        "field", "hill", "field", "field", // D-row (approach)
        "field", "field", "field", // E-row (forward)
        "field", "field", "field", "field", // F-row (attacker start)
    ];
    // shuffle B-row terrain (indices 7-10) among themselves
    for (let i = 10; i > 7; i--) {
        const j = 7 + rand(i - 7 + 1);
        [pool[i], pool[j]] = [pool[j], pool[i]];
    }
    // light randomization on other rows
    if (d6() >= 4)
        pool[4 + rand(3)] = ["hill", "village"][rand(2)];
    if (d6() >= 5)
        pool[11 + rand(3)] = "hill";
    tiles = pool.map((t, i) => {
        const info = TERRAIN_DATA.find(x => x.name === t);
        return { index: i, terrain: t, mul: info.mul };
    });
    // place defenders on best adjacent B-row tiles
    const bRow = [7, 8, 9, 10];
    bRow.sort((a, b) => tiles[b].mul - tiles[a].mul);
    const defTiles = [bRow[0]];
    for (const t of bRow) {
        if (defTiles.length >= 3)
            break;
        if (!defTiles.includes(t) && defTiles.some(dt => ADJ[dt].includes(t)))
            defTiles.push(t);
    }
    for (const t of bRow) {
        if (defTiles.length >= 3)
            break;
        if (!defTiles.includes(t))
            defTiles.push(t);
    }
    units = [
        mkBn("I", "394th", "atk", 820, 700, 12, 6, 85, 21),
        mkBn("II", "394th", "atk", 790, 670, 12, 6, 80, 22),
        mkBn("III", "394th", "atk", 850, 740, 14, 8, 90, 23),
        mkBn("I", "1028th", "def", 680, 580, 8, 4, 70, defTiles[0]),
        mkBn("II", "1028th", "def", 720, 620, 10, 6, 75, defTiles[1]),
        mkBn("III", "1028th", "def", 650, 560, 8, 4, 65, defTiles[2]),
    ];
    // one defender forward as outpost
    const defIdx = rand(3);
    units[3 + defIdx].tile = [11, 12, 13][rand(3)];
    units[3 + defIdx].entrenched = false;
    atkSupport = { hmg: 8, hmortar: 4, at: 4 };
    atkGuns = 12;
    defGuns = 8;
    turn = 1;
    over = false;
    scoutCount.clear();
    fullLog = [];
    simulateAll();
    stepIdx = 0;
    renderFrame();
}
function mkBn(name, rgt, side, men, rifles, mg, mortar, morale, tile) {
    return {
        id: name + "/" + rgt.replace("th", ""),
        name, rgt, side, type: "inf", size: "bn",
        men, rifles, mg, mortar, at: 0, morale,
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
    addLog(`394th Support: ${atkSupport.hmg} HMG, ${atkSupport.hmortar}x 120mm, ${atkSupport.at} AT guns`, "info");
    addLog(`1028th: ${def.reduce((s, u) => s + u.men, 0)} men  CV:${def.reduce((s, u) => s + cv(u), 0)}  |  Off-map: ${defGuns}x 76mm  |  ENTRENCHED`, "info");
    addLog("Objective: breach the main line and reach the A-row.", "info");
    steps.push(snap());
    while (!over) {
        turn++;
        for (const u of units)
            if (!u.routed)
                u.suppression = Math.max(0, u.suppression - 1);
        addLog(`\n==== ${hourStr(turn)} HRS ====`, "hdr");
        // Turn 1: detach recon platoons
        if (turn === 1) {
            for (const bn of units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed)) {
                const r = detachRecon(bn);
                units.push(r);
                addLog(`${r.id} detaches from ${bn.id} (${r.men} men, ${r.mg} MG)`, "recon");
            }
        }
        decideTurnActions();
        allocateSupport();
        // Scouting bns: advance recon forward
        doReconAdvance();
        steps.push(snap(true));
        // Recon spots and returns
        doReconSpotAndReturn();
        // Artillery -- both sides fire when they have targets
        fireAtkArt();
        counterBattery();
        // Each bn: assault adjacent enemies or advance
        doCombatAndMovement();
        // Defender artillery interdiction
        fireDefArt();
        // Return support weapons to regimental pool (with attrition)
        detachSupport();
        checkEnd();
        steps.push(snap());
    }
}
// -- Turn decisions -----------------------------------------------
function decideTurnActions() {
    scoutingBns.clear();
    const anyRevealed = units.some(u => u.side === "def" && !u.routed && u.revealed);
    for (const bn of units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed)) {
        const sameEnemy = units.some(u => u.side === "def" && u.tile === bn.tile && !u.routed);
        const prev = scoutCount.get(bn.id) || 0;
        // Move if: in contact, OR any defender is revealed, OR scouted 2+ turns already
        if (sameEnemy || anyRevealed || prev >= 2) {
            addLog(`${bn.id}: advancing`, "info");
            scoutCount.set(bn.id, 0);
            continue;
        }
        // Otherwise send recon patrol
        scoutingBns.add(bn.id);
        scoutCount.set(bn.id, prev + 1);
        addLog(`${bn.id}: orders recon patrol ahead`, "recon");
    }
}
// -- Support allocation -------------------------------------------
function allocateSupport() {
    // Assign regimental support weapons to the main effort battalion
    // Priority: bn closest to enemy, or bn about to assault
    supportTarget = "";
    if (atkSupport.hmg <= 0 && atkSupport.hmortar <= 0 && atkSupport.at <= 0)
        return;
    const candidates = units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed && !scoutingBns.has(u.id));
    if (!candidates.length)
        return;
    // pick bn with adjacent revealed enemy (closest to contact), breaking ties by lowest row
    let best = null;
    let bestScore = -1;
    for (const bn of candidates) {
        const adjEnemy = ADJ[bn.tile].filter(t => units.some(u => u.side === "def" && u.tile === t && !u.routed && u.revealed));
        const sameEnemy = units.some(u => u.side === "def" && u.tile === bn.tile && !u.routed);
        const score = (sameEnemy ? 100 : 0) + adjEnemy.length * 10 + (6 - ROW[bn.tile]);
        if (score > bestScore) {
            bestScore = score;
            best = bn;
        }
    }
    if (!best)
        return;
    supportTarget = best.id;
    // temporarily attach support weapons to the bn for this hour
    best.mg += atkSupport.hmg;
    best.mortar += atkSupport.hmortar;
    best.at += atkSupport.at;
    addLog(`Support wpns -> ${best.id}: +${atkSupport.hmg} HMG, +${atkSupport.hmortar}x 120mm, +${atkSupport.at} AT`, "info");
}
function detachSupport() {
    // remove support weapons from the bn after actions resolve
    if (!supportTarget)
        return;
    const bn = units.find(u => u.id === supportTarget);
    if (bn) {
        // support weapons may have been lost to casualties — clamp
        const lostHmg = Math.max(0, atkSupport.hmg - bn.mg);
        const lostMor = Math.max(0, atkSupport.hmortar - bn.mortar);
        const lostAt = Math.max(0, atkSupport.at - bn.at);
        bn.mg = Math.max(0, bn.mg - atkSupport.hmg);
        bn.mortar = Math.max(0, bn.mortar - atkSupport.hmortar);
        bn.at = Math.max(0, bn.at - atkSupport.at);
        if (lostHmg + lostMor + lostAt > 0) {
            atkSupport.hmg = Math.max(0, atkSupport.hmg - lostHmg);
            atkSupport.hmortar = Math.max(0, atkSupport.hmortar - lostMor);
            atkSupport.at = Math.max(0, atkSupport.at - lostAt);
            addLog(`Support losses: ${lostHmg ? `-${lostHmg} HMG ` : ""}${lostMor ? `-${lostMor} mortar ` : ""}${lostAt ? `-${lostAt} AT` : ""}(pool: ${atkSupport.hmg}/${atkSupport.hmortar}/${atkSupport.at})`, "info");
        }
    }
    else {
        // bn was routed while carrying support — lose some
        if (d6() <= 3) {
            atkSupport.hmg = Math.max(0, atkSupport.hmg - 1);
        }
        if (d6() <= 2) {
            atkSupport.hmortar = Math.max(0, atkSupport.hmortar - 1);
        }
        if (d6() <= 2) {
            atkSupport.at = Math.max(0, atkSupport.at - 1);
        }
        addLog(`Support wpns lost with routed bn (pool: ${atkSupport.hmg}/${atkSupport.hmortar}/${atkSupport.at})`, "result");
    }
    supportTarget = "";
}
// -- Recon --------------------------------------------------------
function doReconAdvance() {
    // Advance recon platoons forward (only for scouting battalions)
    const recons = units.filter(u => u.type === "recon" && u.side === "atk" && !u.routed
        && u.parent && scoutingBns.has(u.parent));
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
}
function doReconSpotAndReturn() {
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
    // Recon returns to parent after scouting
    for (const r of units.filter(u => u.type === "recon" && u.side === "atk" && !u.routed)) {
        const par = units.find(u => u.id === r.parent);
        if (par && !par.routed && r.tile !== par.tile) {
            const from = r.tile;
            r.tile = par.tile;
            addLog(`${r.id} returns to ${par.id} at ${LABELS[r.tile]}`, "recon");
        }
    }
}
// -- ARTILLERY (callable from any phase) -------------------------
function fireAtkArt() {
    if (atkGuns <= 0)
        return;
    const tgt = new Set();
    for (const u of units)
        if (u.side === "def" && u.revealed && !u.routed)
            tgt.add(u.tile);
    if (tgt.size === 0)
        return;
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
function fireDefArt() {
    if (defGuns <= 0)
        return;
    const atkTiles = [];
    for (const u of units)
        if (u.side === "atk" && u.type === "inf" && !u.routed && !atkTiles.includes(u.tile))
            atkTiles.push(u.tile);
    if (atkTiles.length === 0)
        return;
    const ti = atkTiles[rand(atkTiles.length)];
    const tgts = units.filter(u => u.side === "atk" && u.tile === ti && !u.routed);
    for (const a of tgts) {
        const cas = Math.max(1, Math.floor(rand(2, 5) * defGuns / Math.max(1, tgts.length) / tiles[ti].mul));
        applyCas(a, cas);
        addLog(`1028th Art (${defGuns}x 76mm) -> ${LABELS[ti]}: ${a.id} -${cas} men`, "arty");
    }
}
function counterBattery() {
    if (atkGuns > 0 && defGuns > 0 && d6() >= 5) {
        defGuns--;
        addLog(`Counter-battery: 1028th Art loses 1 gun (${defGuns} left)`, "arty");
    }
    if (defGuns > 0 && atkGuns > 0 && d6() >= 5) {
        atkGuns--;
        addLog(`Counter-battery: 394th Art loses 1 gun (${atkGuns} left)`, "arty");
    }
}
// -- COMBAT & MOVEMENT (per-unit decisions, no phases) -----------
function doCombatAndMovement() {
    // Recon platoons withdraw from contested tiles
    for (const r of units.filter(u => u.type === "recon" && !u.routed)) {
        if (units.some(u => u.side !== r.side && u.tile === r.tile && !u.routed)) {
            const back = ADJ[r.tile].filter(t => r.side === "atk" ? ROW[t] > ROW[r.tile] : ROW[t] < ROW[r.tile]);
            if (back.length) {
                const from = r.tile;
                r.tile = back[rand(back.length)];
                addLog(`${r.id} withdraws: ${LABELS[from]} -> ${LABELS[r.tile]}`, "move");
            }
        }
    }
    // Each non-scouting bn either assaults or advances
    const plan = new Map();
    const advancers = [];
    for (const a of units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed
        && !scoutingBns.has(u.id))) {
        // Forward targets first, then lateral flanking
        let targets = ADJ[a.tile]
            .filter(t => ROW[t] < ROW[a.tile])
            .filter(t => units.some(u => u.side === "def" && u.tile === t && !u.routed && u.revealed));
        if (units.some(u => u.side === "def" && u.tile === a.tile && !u.routed))
            targets.push(a.tile);
        if (targets.length === 0) {
            targets = ADJ[a.tile]
                .filter(t => ROW[t] === ROW[a.tile])
                .filter(t => units.some(u => u.side === "def" && u.tile === t && !u.routed && u.revealed));
            if (targets.length > 0)
                addLog(`${a.id} flanks from ${LABELS[a.tile]}`, "combat");
        }
        if (targets.length) {
            const best = targets.reduce((a2, b2) => {
                const s = (ti) => units.filter(u => u.side === "def" && u.tile === ti && !u.routed).reduce((x, u) => x + cv(u), 0);
                return s(b2) < s(a2) ? b2 : a2;
            });
            if (!plan.has(best))
                plan.set(best, []);
            plan.get(best).push(a);
        }
        else {
            advancers.push(a);
        }
    }
    // Execute assaults
    const capturedTiles = new Set();
    if (plan.size > 0) {
        // record attacker positions before assault
        const posBefore = new Map();
        for (const u of units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed))
            posBefore.set(u.id, u.tile);
        const attackedTiles = new Set(plan.keys());
        for (const [ti, atks] of plan) {
            const ds = units.filter(u => u.side === "def" && u.tile === ti && !u.routed);
            if (ds.length > 0)
                resolveCombat(atks, ds, ti, attackedTiles);
        }
        // detect tiles just captured
        for (const u of units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed)) {
            const prev = posBefore.get(u.id);
            if (prev !== undefined && prev !== u.tile)
                capturedTiles.add(u.tile);
        }
    }
    // -- Defender counterattack: only against isolated enemy on captured tiles --
    if (capturedTiles.size > 0) {
        for (const d of units.filter(u => u.side === "def" && !u.routed && u.morale > 60)) {
            if (units.some(u => u.side === "atk" && u.tile === d.tile && !u.routed))
                continue;
            // only counterattack adjacent captured tiles
            const targets = ADJ[d.tile].filter(t => capturedTiles.has(t) && ROW[t] >= ROW[d.tile]);
            if (!targets.length)
                continue;
            // only counterattack if the enemy there is isolated (single bn, no adjacent friendly atk support)
            const target = targets.find(t => {
                const eOnTile = units.filter(u => u.side === "atk" && u.type === "inf" && u.tile === t && !u.routed);
                if (eOnTile.length !== 1)
                    return false; // don't attack multiple bns
                const adjAtkSupport = ADJ[t].filter(a => a !== d.tile &&
                    units.some(u => u.side === "atk" && u.type === "inf" && u.tile === a && !u.routed));
                return adjAtkSupport.length === 0; // only if enemy is alone
            });
            if (!target)
                continue;
            const enemy = units.filter(u => u.side === "atk" && u.type === "inf" && u.tile === target && !u.routed);
            if (!enemy.length)
                continue;
            // only if we have a decent chance (CV ratio > 0.6)
            if (cv(d) / enemy.reduce((s, u) => s + cv(u), 0) < 0.6)
                continue;
            addLog(`${d.id} counterattacks isolated ${enemy[0].id} at ${LABELS[target]}!`, "combat");
            resolveCounterattack(d, enemy, target);
            capturedTiles.delete(target);
        }
    }
    // Advance non-assaulting bns
    for (const a of advancers) {
        if (units.some(u => u.side === "def" && u.tile === a.tile && !u.routed))
            continue;
        const latEnemy = ADJ[a.tile]
            .filter(t => ROW[t] === ROW[a.tile])
            .some(t => units.some(u => u.side === "def" && u.tile === t && !u.routed));
        if (latEnemy) {
            addLog(`${a.id} holds ${LABELS[a.tile]} -- flanking adjacent defenders`, "move");
            continue;
        }
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
        for (const r of units.filter(u => u.type === "recon" && u.parent === a.id && !u.routed)) {
            r.tile = a.tile;
        }
        addLog(`${a.id} advances ${LABELS[from]} -> ${LABELS[a.tile]}`, "move");
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
    }
}
function resolveCombat(atks, defs, ti, attackedTiles) {
    const tile = tiles[ti];
    const atkCV = atks.reduce((s, a) => s + cv(a), 0);
    const defCV = defs.reduce((s, d) => s + cv(d), 0);
    if (atkCV <= 0 || defCV <= 0)
        return;
    const isFlankAssault = atks.some(a => ROW[a.tile] === ROW[ti] && a.tile !== ti);
    const flankMul = isFlankAssault ? 1.2 : 1.0;
    if (isFlankAssault)
        addLog(`  flanking assault: x1.2 attacker CV`, "combat");
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
    const entMul = defs.some(d => d.entrenched) ? 1.3 : 1.0;
    if (entMul > 1)
        addLog(`  defenders entrenched: x${entMul} bonus`, "combat");
    const tAmen = atks.reduce((s, a) => s + a.men, 0);
    const rawRatio = (atkCV * flankMul) / totalDefCV;
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
        if (d.morale <= 25 || d.men < 100) {
            const rear = ADJ[d.tile].filter(t => ROW[t] < ROW[d.tile] &&
                !units.some(u2 => u2.side === "atk" && u2.tile === t && !u2.routed));
            if (rear.length && d.morale > 10 && d.men >= 40) {
                const from = d.tile;
                d.tile = rear[rand(rear.length)];
                d.entrenched = false;
                addLog(`  ${d.id} RETREATS ${LABELS[from]} -> ${LABELS[d.tile]} (M:${d.morale})`, "move");
            }
            else {
                d.routed = true;
                addLog(`  X ${d.id} ROUTS (${d.men} flee)`, "result");
            }
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
function resolveCounterattack(ctr, enemy, ti) {
    const tile = tiles[ti];
    const cCV = cv(ctr);
    const eCV = enemy.reduce((s, u) => s + cv(u), 0);
    if (cCV <= 0 || eCV <= 0)
        return;
    // no entrenchment — attackers just arrived
    const ratio = cCV / eCV;
    const eff = ratio / tile.mul;
    let cr, er;
    if (eff >= 2.0) {
        cr = 0.02;
        er = 0.07;
    }
    else if (eff >= 1.2) {
        cr = 0.04;
        er = 0.04;
    }
    else if (eff >= 0.8) {
        cr = 0.06;
        er = 0.025;
    }
    else {
        cr = 0.08;
        er = 0.015;
    }
    const cLoss = Math.max(1, Math.floor(ctr.men * cr * (0.8 + Math.random() * 0.4)));
    applyCas(ctr, cLoss);
    ctr.morale = clamp(ctr.morale - Math.ceil(cLoss / 8), 0, 100);
    let eLoss = 0;
    const tMen = enemy.reduce((s, u) => s + u.men, 0);
    for (const e of enemy) {
        const l = Math.max(1, Math.round(tMen * er * (e.men / Math.max(1, tMen)) * (0.8 + Math.random() * 0.4)));
        applyCas(e, l);
        e.morale = clamp(e.morale - Math.ceil(l / 10), 0, 100);
        eLoss += l;
        if (e.morale <= 15 || e.men < 80) {
            e.routed = true;
            addLog(`  X ${e.id} ROUTS from counterattack`, "result");
        }
    }
    const eAlive = enemy.filter(e => !e.routed);
    let result;
    if (!eAlive.length) {
        ctr.tile = ti;
        ctr.entrenched = false;
        result = `RETAKES ${LABELS[ti]}`;
    }
    else if (eff >= 1.3 && d6() >= 4) {
        for (const e of eAlive) {
            const back = ADJ[e.tile].filter(t => ROW[t] > ROW[e.tile]);
            if (back.length)
                e.tile = back[rand(back.length)];
        }
        ctr.tile = ti;
        ctr.entrenched = false;
        result = `drives enemy from ${LABELS[ti]}`;
    }
    else {
        result = `counterattack repelled`;
    }
    if (ctr.morale <= 25 || ctr.men < 100) {
        ctr.routed = true;
        addLog(`  X ${ctr.id} ROUTS after counterattack`, "result");
    }
    addLog(`  ${ctr.id} -> ${LABELS[ti]}: ${result}  [CV ${ratio.toFixed(1)}:1 -> ${eff.toFixed(1)} eff]  C-${cLoss} E-${eLoss}`, "combat");
}
// -- Victory ------------------------------------------------------
function checkEnd() {
    const breach = units.filter(u => u.side === "atk" && !u.routed && u.type === "inf" && ROW[u.tile] <= 1);
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
    else if (turn >= 12) {
        over = true;
        addLog(`\n* STALEMATE -- nightfall (1800)`, "result");
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
        + `<div class="offmap-sym">${symSvg(SIDC_DEF_ART, 24)}</div>`
        + `<div class="offmap-info"><div class="offmap-nm">1028th Art Btry</div>`
        + `<div class="offmap-det">${f.defGuns}x 76mm | ${f.defGuns > 0 ? "READY" : "DESTROYED"}</div></div></div>`;
    // zone labels
    for (let r = 0; r < 7; r++) {
        const y = MAP_PAD_TOP + r * RS + Math.floor(HH / 2) - 6;
        const cls = r <= 2 ? "zone zone-def" : r >= 4 ? "zone zone-atk" : "zone zone-nml";
        html += `<div class="${cls}" style="top:${y}px">${ZONES[r]}</div>`;
    }
    // hexes
    for (let i = 0; i < N_TILES; i++) {
        const p = HP[i], t = tiles[i];
        const onTile = f.units.filter(u => u.tile === i && !u.routed);
        const hasAtk = onTile.some(u => u.side === "atk");
        const hasDef = onTile.some(u => u.side === "def");
        const contested = hasAtk && hasDef;
        const edge = ROW[i] === 0 || ROW[i] === 6;
        html += `<div class="hex-wrap${edge ? " hex-edge" : ""}" style="left:${p.x}px;top:${p.y}px;width:${HW}px;height:${HH}px">`
            + `<div class="hex-bdr${contested ? " contested" : ""}" style="clip-path:${CLIP}"></div>`
            + `<div class="hex-fill ter-${t.terrain}" style="clip-path:${CLIP}"></div>`
            + `<div class="hex-info" style="clip-path:${CLIP}">`
            + `<span class="hex-id">${LABELS[i]}</span>`
            + `<span class="hex-ter">${esc(t.terrain)} x${t.mul}</span>`
            + `</div>`
            + `<div class="hex-units">${mkUnits(onTile, f.reconOut)}</div>`
            + `</div>`;
    }
    // off-map attacker art (bottom)
    const botY = MAP_PAD_TOP + 6 * RS + HH + 8;
    html += `<div class="offmap offmap-atk" style="top:${botY}px">`
        + `<div class="offmap-sym">${symSvg(SIDC_ATK_ART, 24)}</div>`
        + `<div class="offmap-info"><div class="offmap-nm">394th Art Bn</div>`
        + `<div class="offmap-det">${f.atkGuns}x 105mm | ${f.atkGuns > 0 ? "READY" : "DESTROYED"}</div></div></div>`;
    // recon arrows (parent -> recon tile) during recon phase
    if (f.reconOut) {
        const rcns = f.units.filter(u => u.type === "recon" && !u.routed && u.parent);
        for (const r of rcns) {
            const par = f.units.find(u => u.id === r.parent);
            if (!par || par.tile === r.tile)
                continue;
            const from = HP[par.tile], to = HP[r.tile];
            const x1 = from.x + HW / 2, y1 = from.y + HH / 2;
            const x2 = to.x + HW / 2, y2 = to.y + HH / 2;
            html += `<svg class="rcn-arrow" style="position:absolute;left:0;top:0;width:100%;height:100%;pointer-events:none;z-index:20">`
                + `<defs><marker id="ah" markerWidth="8" markerHeight="6" refX="7" refY="3" orient="auto">`
                + `<path d="M0,0 L8,3 L0,6" fill="#2080c0"/></marker></defs>`
                + `<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="#2080c0" stroke-width="2" stroke-dasharray="6,3" marker-end="url(#ah)"/>`
                + `</svg>`;
        }
    }
    // scale indicator
    const scaleY = MAP_PAD_TOP + 6 * RS + HH + 46;
    html += `<div class="map-scale" style="top:${scaleY}px">1 hex ~ 500m across</div>`;
    g.innerHTML = html;
}
function mkUnits(on, reconOut) {
    // recon only visible when deployed (reconOut snapshot)
    const visible = on.filter(u => !u.routed);
    const sorted = [...visible].sort((a, b) => {
        if (a.side !== b.side)
            return a.side === "def" ? -1 : 1;
        return 0;
    });
    return sorted.map((u, i) => mkUnit(u, 10 + i)).join("");
}
function mkUnit(u, z) {
    const fog = !u.revealed && u.side === "def";
    const sup = u.suppression > 0 ? ` u-s${u.suppression}` : "";
    const cls = u.side === "atk" ? "u-atk" : "u-def";
    const szPx = u.size === "plt" ? 14 : 18;
    if (fog) {
        return `<div class="unit ${cls} u-fog" style="z-index:${z}" title="Unidentified enemy unit">`
            + `<div class="u-sym">${symSvg(SIDC_UNK, 16)}</div>`
            + `<div class="u-label">???</div></div>`;
    }
    const ent = u.entrenched ? " ENT" : "";
    const st = u.suppression > 0 ? " " + SUPP[u.suppression].substring(0, 4) : "";
    const tip = `${u.id}  ${u.men} men  CV:${cv(u)}  ${u.mg}MG ${u.mortar}Mor ${u.at}AT  M:${u.morale}  ${SUPP[u.suppression]}${u.entrenched ? "  ENTRENCHED" : ""}`;
    const rcn = u.type === "recon";
    return `<div class="unit ${cls}${sup}${rcn ? " u-rcn" : ""}" style="z-index:${z}" title="${esc(tip)}">`
        + `<div class="u-sym">${symSvg(u.sidc, szPx)}</div>`
        + `<div class="u-label"><div class="u-name">${u.id}</div>${u.men} CV:${cv(u)}${ent}${st}</div>`
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
        return `<tr class="oob-tr fog"><td class="oob-bn ${indent}">${esc(u.id)}</td><td colspan="10" class="oob-unk">???</td></tr>`;
    const cls = u.routed ? "oob-tr rt" : "oob-tr";
    const sup = u.suppression > 0 && !u.routed ? ` s${u.suppression}` : "";
    const typeStr = u.type === "recon" ? "RCN" : "INF";
    return `<tr class="${cls}${sup}">`
        + `<td class="oob-bn ${indent}">${esc(u.id)}</td>`
        + `<td class="oob-ty">${typeStr}</td>`
        + `<td class="oob-n">${u.routed ? "--" : u.men}</td>`
        + `<td class="oob-n">${u.mg}</td>`
        + `<td class="oob-n">${u.mortar}</td>`
        + `<td class="oob-n">${u.at}</td>`
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
// -- Status / controls --------------------------------------------
function renderPhaseBar(f) {
    const label = f.over ? "RESULT" : hourStr(f.turn);
    $("phase").textContent = label;
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
    const fs = steps[steps.length - 1];
    let h = `<div class="res-v">${esc(verdict)}</div>`;
    h += `<div class="res-sub">${hourStr(1)}–${hourStr(fs.turn)} (${fs.turn} hrs)  |  Atk Art: ${fs.atkGuns}/${steps[0].atkGuns} guns  |  Def Art: ${fs.defGuns}/${steps[0].defGuns} guns  |  Spt: ${fs.atkSupport.hmg}/${steps[0].atkSupport.hmg} HMG, ${fs.atkSupport.hmortar}/${steps[0].atkSupport.hmortar} Mor, ${fs.atkSupport.at}/${steps[0].atkSupport.at} AT</div>`;
    for (const side of ["atk", "def"]) {
        const label = side === "atk" ? "394th Infantry Regiment" : "1028th Rifle Regiment";
        h += `<div class="res-rgt">${esc(label)}</div>`;
        h += `<table class="res-t"><thead><tr><th>Unit</th><th>Type</th><th>Men</th><th></th><th>End</th><th>Lost</th><th>MG</th><th>Mor</th><th>AT</th><th>CV</th></tr></thead><tbody>`;
        const sideUnits = ini.filter(u => u.side === side);
        for (const iu of sideUnits) {
            const fu = fin.find(u => u.id === iu.id);
            if (!fu)
                continue;
            const lost = iu.men - Math.max(0, fu.men);
            const r = fu.routed ? ' class="rt"' : '';
            const typeStr = iu.type === "recon" ? "RCN" : "INF";
            h += `<tr${r}><td>${esc(iu.id)}</td><td>${typeStr}</td><td>${iu.men}</td><td>-></td>`
                + `<td>${fu.routed ? "0" : Math.max(0, fu.men)}</td>`
                + `<td class="res-lost">-${lost}</td>`
                + `<td>${iu.mg}->${fu.mg}</td><td>${iu.mortar}->${fu.mortar}</td><td>${iu.at}->${fu.at}</td>`
                + `<td>${cv(iu)}->${cv(fu)}</td></tr>`;
        }
        const s0 = sideUnits.reduce((s, u) => s + u.men, 0);
        const s1 = fin.filter(u => u.side === side).reduce((s, u) => u.routed ? s : s + Math.max(0, u.men), 0);
        const cv0 = sideUnits.reduce((s, u) => s + cv(u), 0);
        const cv1 = fin.filter(u => u.side === side).reduce((s, u) => u.routed ? s : s + cv(u), 0);
        h += `<tr class="res-tot"><td>Total</td><td></td><td>${s0}</td><td>-></td><td>${s1}</td><td class="res-lost">-${s0 - s1}</td><td></td><td></td><td></td><td>${cv0}->${cv1}</td></tr>`;
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
// Keyboard hotkeys
document.addEventListener("keydown", (e) => {
    if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement)
        return;
    switch (e.key) {
        case "ArrowRight":
        case "d":
        case ".":
            if (stepIdx < steps.length - 1) {
                stepIdx++;
                renderFrame();
            }
            e.preventDefault();
            break;
        case "ArrowLeft":
        case "a":
        case ",":
            if (stepIdx > 0) {
                stepIdx--;
                renderFrame();
            }
            e.preventDefault();
            break;
        case "End":
        case "s":
            stepIdx = steps.length - 1;
            renderFrame();
            e.preventDefault();
            break;
        case "Home":
        case "w":
            stepIdx = 0;
            renderFrame();
            e.preventDefault();
            break;
        case "r":
            initBattle();
            e.preventDefault();
            break;
    }
});
initBattle();
