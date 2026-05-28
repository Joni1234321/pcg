"use strict";
// =================================================================
//  BATTLES -- Regiment Combat on a 4x8 Hex Grid
// =================================================================
//  milsymbol NATO counters, recon detachments, off-map artillery,
//  flanking fire, entrenchment.  No emojis -- text only.
// =================================================================
// -- Grid constants -----------------------------------------------
// 4 columns × 8 rows = 32 tiles, flat-top hexes ("odd-q" offset: odd cols shifted down)
// Row A = top (defender rear), Row H = bottom (attacker rear)
// Defenders on rows C-D (main line + forward screen), attackers on rows E-F.
const COLS = 4, ROWS = 8;
const N_TILES = COLS * ROWS;
const ROW_NAMES = ["A", "B", "C", "D", "E", "F", "G", "H"];
const LABELS = [];
for (let r = 0; r < ROWS; r++) {
    for (let c = 0; c < COLS; c++)
        LABELS.push(ROW_NAMES[r] + (c + 1));
}
function tileIdx(c, r) { return r * COLS + c; }
function colOf(i) { return i % COLS; }
const ROW = [];
for (let i = 0; i < N_TILES; i++)
    ROW.push(Math.floor(i / COLS));
// flat-top "odd-q" offset neighbour formula
const ADJ = [];
for (let i = 0; i < N_TILES; i++) {
    const c = colOf(i), r = ROW[i];
    const odd = (c % 2) === 1; // odd columns are offset DOWN by half a hex
    const nbrs = odd
        ? [[c, r - 1], [c, r + 1], [c - 1, r], [c - 1, r + 1], [c + 1, r], [c + 1, r + 1]]
        : [[c, r - 1], [c, r + 1], [c - 1, r - 1], [c - 1, r], [c + 1, r - 1], [c + 1, r]];
    ADJ.push(nbrs.filter(([nc, nr]) => nc >= 0 && nc < COLS && nr >= 0 && nr < ROWS)
        .map(([nc, nr]) => tileIdx(nc, nr)));
}
// Zone labels — maneuver warfare (everything under attack from turn 1)
const ZONES = [
    "DEFENDER REAR", // A — deep rear
    "DEFENSE IN DEPTH", // B — fallback line
    "MAIN DEFENSE LINE", // C — main defense
    "FORWARD SCREEN", // D — delay positions
    "NO MANS LAND", // E — contested approach
    "ATTACK ASSEMBLY", // F — attacker front
    "SUPPORT", // G — attacker support
    "ATTACKER REAR", // H
];
const TERRAIN_DATA = [
    { name: "field", mul: 1.0 },
    { name: "hill", mul: 1.5 },
    { name: "village", mul: 1.8 },
    { name: "forest", mul: 2.0 },
    { name: "ridge", mul: 1.7 },
];
const SUPP = ["READY", "DISRUPTED", "SUPPRESSED", "PINNED"];
// SIDC codes (15-char, MIL-STD-2525C). Position 12 = echelon; "E" = company.
const SIDC_ATK_INF = "SFGPUCI----E---"; // friendly infantry company
const SIDC_DEF_INF = "SHGPUCI----E---"; // hostile infantry company
const SIDC_ATK_RCN = "SFGPUCR----D---"; // friendly recon plt
const SIDC_ATK_SPT = "SFGPUCI----D---"; // friendly support plt
const SIDC_ATK_ART = "SFGPUCF----E---"; // friendly arty battery
const SIDC_DEF_ART = "SHGPUCF----E---"; // hostile arty battery
const SIDC_UNK = "SHGPUCI----E---"; // unknown hostile (shown foggy)
// hex layout — flat-top orientation (flat edges at top/bottom, vertices left/right)
// HW = vertex-to-vertex width = 2s, HH = flat-to-flat height = s√3
const HW = 84, HH = 72;
const CS = Math.floor(HW * 0.75) + 4;
const RS = HH + 4;
const HO = Math.floor(RS / 2);
const CLIP = "polygon(25% 0%,75% 0%,100% 50%,75% 100%,25% 100%,0% 50%)";
const MAP_PAD_TOP = 30;
const MAP_PAD_LEFT = 10;
const HP = [];
{
    // Flat-top rectangular grid: every column has ROWS tiles.
    // Odd-indexed columns (1, 3, 5) are offset DOWN by HO = RS/2 so hex centres tessellate.
    for (let r = 0; r < ROWS; r++) {
        for (let c = 0; c < COLS; c++) {
            const off = (c % 2) === 1 ? HO : 0;
            HP[tileIdx(c, r)] = { x: MAP_PAD_LEFT + c * CS, y: MAP_PAD_TOP + off + r * RS };
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
let knownToAtk = []; // per-tile fog state for attacker side
let knownToDef = []; // per-tile fog state for defender side
let viewSide = "atk"; // which side's fog of war to show
let reserveCommitted = false;
let orderSet = 1; // current attacker order set (1 = first obj row, 2 = second, 3 = third)
let orderHoldTurns = 0; // turns remaining before new orders can be issued (delay after cancel)
let stallTurns = 0; // consecutive turns with no forward progress
let defOrderSet = 1; // defender order set (1 = hold forward, 2 = fallback to C, 3 = last stand B/A)
let defOrderHoldTurns = 0; // delay before new defender orders
// -- Helpers ------------------------------------------------------
function rand(a, b) {
    if (b === undefined)
        return Math.floor(Math.random() * a);
    return a + Math.floor(Math.random() * (b - a + 1));
}
function d6() { return rand(1, 6); }
function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }
// 30-minute turns. Turn 1 = 06:00, turn 2 = 06:30, ...
function tStr(t) {
    const totalMin = 6 * 60 + (t - 1) * 30;
    const hh = Math.floor(totalMin / 60), mm = totalMin % 60;
    return String(hh).padStart(2, '0') + String(mm).padStart(2, '0');
}
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
    return {
        turn, reconOut,
        units: units.map(u => ({ ...u })),
        owners: tiles.map(t => t.owner),
        known: knownToAtk.slice(),
        knownDef: knownToDef.slice(),
        objectives: units.map(u => u.objective ?? -1),
        orderSet, defOrderSet,
        atkGuns, defGuns, logEnd: fullLog.length, over,
    };
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
// -- Init & Simulate ----------------------------------------------
// Stacking capacity by terrain: forest/ridge can hide 2 companies, others only 1.
function capacityOf(terrain) {
    return (terrain === "forest" || terrain === "ridge") ? 2 : 1;
}
function initBattle() {
    _svgCache.clear();
    // Generate terrain per row (COLS=4 entries each).
    // Defender half (rows 0-2): strong cover; attacker half (rows 5-7): open ground.
    const TERR_DEF = ["forest", "ridge", "village", "hill"];
    const TERR_MID = ["field", "hill", "forest", "field"];
    const TERR_ATK = ["field", "field", "hill", "field"];
    const pool = [];
    for (let r = 0; r < ROWS; r++) {
        const tab = r <= 2 ? TERR_DEF : r >= 5 ? TERR_ATK : TERR_MID;
        const shuf = tab.slice();
        for (let i = shuf.length - 1; i > 0; i--) {
            const j = rand(i + 1);
            [shuf[i], shuf[j]] = [shuf[j], shuf[i]];
        }
        for (let c = 0; c < COLS; c++)
            pool.push(shuf[c % shuf.length]);
    }
    tiles = pool.map((t, i) => {
        const info = TERRAIN_DATA.find(x => x.name === t);
        const owner = ROW[i] >= 5 ? "atk" : ROW[i] <= 3 ? "def" : "neutral";
        return { index: i, terrain: t, mul: info.mul, owner };
    });
    units = [];
    // --- Deploy defenders ---
    // Main defense line: row C (2) = 4 cos, one per tile, HOLD stance.
    // Forward screen: row D (3) = 2 cos on best-cover tiles, DELAY stance.
    // Reserve: row B (1) = 3 cos (1 per bn).
    const rowCTiles = [...Array(N_TILES).keys()]
        .filter(i => ROW[i] === 2); // main defense line — full blocking
    const rowDTiles = [...Array(N_TILES).keys()]
        .filter(i => ROW[i] === 3)
        .sort((a, b) => tiles[b].mul - tiles[a].mul); // best cover first
    const rowBTiles = [...Array(N_TILES).keys()]
        .filter(i => ROW[i] === 1);
    // 6 forward cos: 4 on row C (full blocking) + 2 on row D (delay positions)
    const defFront = [
        ...rowCTiles, // 4 cos on main line
        rowDTiles[0], // 5th co on best-cover row D tile
        rowDTiles[Math.min(1, rowDTiles.length - 1)], // 6th co
    ];
    const defRear = rowBTiles.slice(0, 3);
    while (defRear.length < 3)
        defRear.push(rowBTiles[0]);
    const defBnNames = ["I", "II", "III"];
    let di = 0;
    for (let bi = 0; bi < defBnNames.length; bi++) {
        const bn = defBnNames[bi];
        for (let n = 1; n <= 3; n++) {
            const tile = n <= 2 ? defFront[di++] : defRear[bi];
            const co = mkCo(bn, n, "1028th", "def", 220, 180, 3, 2, 1, 75, tile);
            // Forward screen (row D) gets DELAY stance, main line (row C) gets HOLD
            if (n <= 2) {
                co.stance = ROW[tile] === 3 ? "delay" : "hold";
                co.objective = tile; // defend this position
                co.objectiveComplete = false;
                co.delayTurns = 0;
            }
            units.push(co);
        }
    }
    // --- Deploy attackers ---
    // 2 assault cos per bn on rows E-F (4-5, close to enemy).
    // 1 reserve co per bn on row G (6, support/follow-on).
    const atkAssaultRows = [4, 5];
    const atkAssaultCands = [...Array(N_TILES).keys()]
        .filter(i => atkAssaultRows.includes(ROW[i]))
        .sort((a, b) => tiles[b].mul - tiles[a].mul);
    const atkAssault = [];
    for (const t of atkAssaultCands) {
        if (atkAssault.length >= 6)
            break;
        const cap = capacityOf(tiles[t].terrain);
        for (let k = 0; k < cap && atkAssault.length < 6; k++)
            atkAssault.push(t);
    }
    while (atkAssault.length < 6)
        atkAssault.push(atkAssaultCands[0]);
    const atkRearCands = [...Array(N_TILES).keys()]
        .filter(i => ROW[i] === 6)
        .sort((a, b) => tiles[a].mul - tiles[b].mul);
    const atkRear = [];
    for (const t of atkRearCands) {
        if (atkRear.length >= 3)
            break;
        atkRear.push(t);
    }
    while (atkRear.length < 3)
        atkRear.push(atkRearCands[0]);
    const atkBnNames = ["I", "II", "III"];
    let ai = 0;
    for (let bi = 0; bi < atkBnNames.length; bi++) {
        const bn = atkBnNames[bi];
        for (let n = 1; n <= 3; n++) {
            const tile = n <= 2 ? atkAssault[ai++] : atkRear[bi];
            units.push(mkCo(bn, n, "394th", "atk", 220, 180, 3, 2, 0, 80, tile));
        }
    }
    // Support platoon
    const leadTile = atkAssault[0];
    units.push({
        id: "SPT/394", name: "SPT", rgt: "394th", side: "atk", type: "support", size: "plt",
        men: 60, rifles: 30, mg: 1, mortar: 4, at: 2, morale: 85,
        suppression: 0, tile: leadTile, revealed: true, routed: false, entrenched: false,
        sidc: SIDC_ATK_SPT, parent: "I/394",
        activity: "Supporting", reorgTurns: 0,
    });
    // --- Issue initial orders ---
    orderSet = 1;
    orderHoldTurns = 0;
    issueOrders();
    // --- Issue initial defender orders ---
    defOrderSet = 1;
    defOrderHoldTurns = 0;
    issueDefOrders();
    reserveCommitted = false;
    knownToAtk = tiles.map(() => false);
    knownToDef = tiles.map((_, i) => ROW[i] <= 4); // defenders know their own territory
    updateOwnership();
    updateFog();
    atkGuns = 12;
    defGuns = 8;
    turn = 1;
    over = false;
    fullLog = [];
    simulateAll();
    stepIdx = 0;
    renderFrame();
}
// -- Attacker orders ----------------------------------------------
function issueOrders() {
    const atkCos = units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed
        && !u.parent?.startsWith("RSV/"));
    // Order set 1: row D tiles (row index 3) — forward screen
    // Order set 2: row C tiles (row index 2) — main defense line
    // Order set 3: row B tiles (row index 1) — breakthrough
    const targetRow = orderSet === 1 ? 3 : orderSet === 2 ? 2 : 1;
    const objTiles = [...Array(N_TILES).keys()].filter(i => ROW[i] === targetRow);
    const rowName = ROW_NAMES[targetRow];
    addLog(`ATK BN CMD: Orders issued — seize row ${rowName}`, "hdr");
    const assigned = new Set();
    for (const co of atkCos) {
        const col = colOf(co.tile);
        let best = -1, bestDist = 999;
        for (const t of objTiles) {
            if (assigned.has(t))
                continue;
            const dist = Math.abs(colOf(t) - col) + Math.abs(ROW[t] - ROW[co.tile]);
            if (dist < bestDist) {
                bestDist = dist;
                best = t;
            }
        }
        if (best === -1) {
            for (const t of objTiles) {
                const dist = Math.abs(colOf(t) - col) + Math.abs(ROW[t] - ROW[co.tile]);
                if (dist < bestDist) {
                    bestDist = dist;
                    best = t;
                }
            }
        }
        co.objective = best;
        co.objectiveComplete = false;
        assigned.add(best);
        addLog(`${co.id} -> OBJ: ${LABELS[best]}`, "info");
    }
    for (const co of units.filter(u => u.side === "atk" && u.parent?.startsWith("RSV/"))) {
        co.objective = undefined;
        co.objectiveComplete = false;
    }
}
// -- Defender orders ----------------------------------------------
function issueDefOrders() {
    const defCos = units.filter(u => u.side === "def" && u.type === "inf" && !u.routed
        && !u.parent?.startsWith("RSV/"));
    if (defOrderSet === 1) {
        // Initial: forward units (row D) DELAY, main line (row C) HOLD
        addLog(`DEF BN CMD: Hold main line (row C), delay on forward screen (row D)`, "hdr");
        for (const co of defCos) {
            co.stance = ROW[co.tile] >= 3 ? "delay" : "hold";
            co.objective = co.tile;
            co.objectiveComplete = false;
            co.delayTurns = 0;
            addLog(`${co.id} -> ${co.stance.toUpperCase()} at ${LABELS[co.tile]}`, "info");
        }
    }
    else if (defOrderSet === 2) {
        // Fallback: all units to row C (main line), HOLD
        addLog(`DEF BN CMD: Fall back to row C — establish new defense line`, "hdr");
        const rowCTiles = [...Array(N_TILES).keys()].filter(i => ROW[i] === 2);
        const assigned = new Set();
        for (const co of defCos) {
            const col = colOf(co.tile);
            let best = -1, bestDist = 999;
            for (const t of rowCTiles) {
                if (assigned.has(t))
                    continue;
                const dist = Math.abs(colOf(t) - col);
                if (dist < bestDist) {
                    bestDist = dist;
                    best = t;
                }
            }
            if (best === -1) {
                for (const t of rowCTiles) {
                    const dist = Math.abs(colOf(t) - col);
                    if (dist < bestDist) {
                        bestDist = dist;
                        best = t;
                    }
                }
            }
            co.stance = "hold";
            co.objective = best;
            co.objectiveComplete = false;
            co.delayTurns = 0;
            assigned.add(best);
            addLog(`${co.id} -> HOLD at ${LABELS[best]}`, "info");
        }
    }
    else {
        // Last stand: fall back to row B
        addLog(`DEF BN CMD: Fall back to row B — last stand`, "hdr");
        const rowBTiles = [...Array(N_TILES).keys()].filter(i => ROW[i] === 1);
        const assigned = new Set();
        for (const co of defCos) {
            const col = colOf(co.tile);
            let best = -1, bestDist = 999;
            for (const t of rowBTiles) {
                if (assigned.has(t))
                    continue;
                const dist = Math.abs(colOf(t) - col);
                if (dist < bestDist) {
                    bestDist = dist;
                    best = t;
                }
            }
            if (best === -1) {
                for (const t of rowBTiles) {
                    const dist = Math.abs(colOf(t) - col);
                    if (dist < bestDist) {
                        bestDist = dist;
                        best = t;
                    }
                }
            }
            co.stance = "hold";
            co.objective = best;
            co.objectiveComplete = false;
            co.delayTurns = 0;
            assigned.add(best);
            addLog(`${co.id} -> HOLD at ${LABELS[best]}`, "info");
        }
    }
}
function checkDefenderOrders() {
    // If orders are on hold, count down
    if (defOrderHoldTurns > 0) {
        defOrderHoldTurns--;
        if (defOrderHoldTurns === 0) {
            addLog(`DEF BN CMD: New orders ready`, "hdr");
            issueDefOrders();
        }
        else {
            addLog(`DEF BN CMD: Issuing new orders... (${defOrderHoldTurns} turns)`, "info");
        }
        return;
    }
    const defCos = units.filter(u => u.side === "def" && u.type === "inf" && !u.routed
        && !u.parent?.startsWith("RSV/"));
    if (!defCos.length)
        return;
    if (defOrderSet === 1) {
        // Check if forward screen (row D) is lost — all delay units retreated or routed
        const fwdUnits = defCos.filter(u => u.stance === "delay");
        const fwdLost = fwdUnits.length === 0 || fwdUnits.every(u => ROW[u.tile] < 3);
        if (fwdLost) {
            defOrderSet = 2;
            defOrderHoldTurns = 2; // takes 2 turns to reorganize
            addLog(`DEF BN CMD: Forward screen lost. Reorganizing... (2 turns)`, "hdr");
        }
    }
    else if (defOrderSet === 2) {
        // Check if main line (row C) is under heavy pressure
        const mainLine = defCos.filter(u => ROW[u.tile] === 2);
        const mainLost = mainLine.length === 0 ||
            mainLine.filter(u => u.morale < 40 || u.men < 120).length >= Math.ceil(mainLine.length / 2);
        const atkOnC = units.some(u => u.side === "atk" && !u.routed && ROW[u.tile] <= 2);
        if (mainLost || atkOnC) {
            defOrderSet = 3;
            defOrderHoldTurns = 2;
            addLog(`DEF BN CMD: Main line breaking. Falling back to row B... (2 turns)`, "hdr");
        }
    }
}
function checkOrderCompletion() {
    // If orders are on hold (bn cmd cancelled, issuing new orders), count down
    if (orderHoldTurns > 0) {
        orderHoldTurns--;
        if (orderHoldTurns === 0) {
            addLog(`BN CMD: New orders ready`, "hdr");
            issueOrders();
        }
        else {
            addLog(`BN CMD: Issuing new orders... (${orderHoldTurns} turns)`, "info");
        }
        return;
    }
    const atkCos = units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed
        && u.objective !== undefined && !u.parent?.startsWith("RSV/"));
    if (!atkCos.length)
        return;
    // Mark objective complete if co is on or past its objective tile
    for (const co of atkCos) {
        if (co.objective !== undefined && !co.objectiveComplete) {
            const onObj = co.tile === co.objective;
            const pastObj = ROW[co.tile] < ROW[co.objective];
            const objClear = !units.some(u => u.side === "def" && u.tile === co.objective && !u.routed);
            if ((onObj || pastObj) && objClear) {
                co.objectiveComplete = true;
                addLog(`${co.id} CONSOLIDATES on ${LABELS[co.tile]} (OBJ ${LABELS[co.objective]} secured)`, "result");
            }
        }
    }
    // Bn cmd can cancel orders if no progress for 4 turns — reissue with 2-turn delay
    const stuckCos = atkCos.filter(u => !u.objectiveComplete);
    if (stuckCos.length > 0 && turn > 4) {
        // Check if any stuck co made progress (moved closer to objective) in the last step
        const prevStep = stepIdx > 0 ? steps[steps.length - 1] : undefined;
        if (prevStep) {
            const noProgress = stuckCos.every(u => {
                const prev = prevStep.units.find(p => p.id === u.id);
                return prev && prev.tile === u.tile;
            });
            // Track consecutive stall turns
            stallTurns = noProgress ? stallTurns + 1 : 0;
            if (stallTurns >= 4) {
                addLog(`BN CMD: Orders cancelled — no progress. Reissuing orders (2 turns)`, "hdr");
                orderHoldTurns = 2;
                stallTurns = 0;
                return;
            }
        }
    }
    // All objectives must be complete (or co routed) to issue new orders
    const allDone = atkCos.every(u => u.objectiveComplete);
    if (!allDone)
        return;
    // Issue next set of orders
    if (orderSet >= 3)
        return; // final orders already active
    orderSet++;
    addLog(`\n** NEW ORDERS — advancing to next objective **`, "hdr");
    issueOrders();
}
function mkCo(bn, num, rgt, side, men, rifles, mg, mortar, at, morale, tile) {
    const name = num + "/" + bn; // e.g. "1/I"
    const rgtShort = rgt.replace("th", "");
    // Co 3 of each bn is the reserve for that bn.
    const isReserve = num === 3;
    return {
        id: name + "/" + rgtShort, // e.g. "1/I/394"
        name, rgt, side, type: "inf", size: "co",
        men, rifles, mg, mortar, at, morale,
        suppression: 0, tile, revealed: side === "atk",
        routed: false, entrenched: side === "def",
        sidc: side === "atk" ? SIDC_ATK_INF : SIDC_DEF_INF,
        parent: (isReserve ? "RSV/" : "") + bn + "/" + rgtShort,
        activity: isReserve ? "In Reserve" : (side === "def" ? "Holding" : "Waiting"),
        reorgTurns: 0,
    };
}
function simulateAll() {
    steps = [];
    const atk = units.filter(u => u.side === "atk");
    const def = units.filter(u => u.side === "def");
    addLog("== 394th Infantry Rgt  vs  1028th Rifle Rgt ==", "hdr");
    addLog(`394th: 9 cos (2 assault + 1 reserve per bn) + SPT plt  CV:${atk.reduce((s, u) => s + cv(u), 0)}`, "info");
    addLog(`1028th: 9 cos (2 front + 1 reserve per bn)  CV:${def.reduce((s, u) => s + cv(u), 0)}  ENTRENCHED`, "info");
    addLog("Maneuver: full contact from turn 1. Reserves commit when bn is under pressure.", "info");
    steps.push(snap());
    while (!over) {
        turn++;
        for (const u of units)
            if (!u.routed)
                u.suppression = Math.max(0, u.suppression - 1);
        // Reset activity labels at start of turn
        for (const u of units) {
            if (u.routed) {
                u.activity = "Routed";
                continue;
            }
            if (u.side === "def") {
                u.activity = u.entrenched ? (u.stance === "delay" ? "Delaying" : "Holding") : "Moving";
            }
            else {
                if (u.parent?.startsWith("RSV/") && !reserveCommitted)
                    u.activity = "In Reserve";
                else if (u.reorgTurns > 0)
                    u.activity = "Reorganizing";
                else
                    u.activity = "Waiting";
            }
        }
        addLog(`\n==== ${tStr(turn)} ====`, "hdr");
        checkReserveCommit();
        // 1. Off-map artillery on revealed targets
        fireAtkArt();
        counterBattery();
        // 2. Movement (with auto-recon + ZOC)
        doMovement();
        // 3. Assaults on adjacent enemies
        doAssaults();
        // 4. Defender interdiction
        fireDefArt();
        // 5. Update tile ownership / fog
        updateOwnership();
        updateFog();
        // 6. Check order completion — consolidate & issue new orders
        checkOrderCompletion();
        // 7. Check defender orders — fallback when lines break
        checkDefenderOrders();
        checkEnd();
        steps.push(snap());
    }
}
// -- Reserve ------------------------------------------------------
function checkReserveCommit() {
    if (reserveCommitted)
        return;
    // Attacker reserves: cos 3 of each bn (parent starts with "RSV/")
    const reserve = units.filter(u => u.side === "atk" && u.parent?.startsWith("RSV/") && !u.routed);
    if (!reserve.length) {
        reserveCommitted = true;
        return;
    }
    const fwdCos = units.filter(u => u.side === "atk" && !u.parent?.startsWith("RSV/") && u.type === "inf");
    const routedCount = fwdCos.filter(u => u.routed).length;
    const allEngaged = fwdCos.filter(u => !u.routed).every(u => units.some(d => d.side === "def" && !d.routed && (d.tile === u.tile || ADJ[u.tile].includes(d.tile))));
    const lateGame = turn >= 6; // 6 × 30 min = 3 h elapsed
    if (routedCount >= 2 || (allEngaged && turn >= 3) || lateGame) {
        reserveCommitted = true;
        // Un-tag reserves so they behave like normal cos and follow the main force
        for (const r of reserve) {
            r.parent = r.parent.replace("RSV/", "");
            // Assign current order objective to newly committed reserve
            const targetRow = orderSet === 1 ? 3 : orderSet === 2 ? 2 : 1;
            const objTiles = [...Array(N_TILES).keys()].filter(i => ROW[i] === targetRow);
            const col = colOf(r.tile);
            let best = objTiles[0];
            let bestDist = 999;
            for (const t of objTiles) {
                const dist = Math.abs(colOf(t) - col);
                if (dist < bestDist) {
                    bestDist = dist;
                    best = t;
                }
            }
            r.objective = best;
            r.objectiveComplete = false;
            addLog(`${r.id} -> OBJ: ${LABELS[best]}`, "info");
        }
        addLog(`Reserves committed! 3rd cos of I/II/III bn advancing.`, "info");
    }
    // Defender reserve: co 3 of each bn commits when adjacent friendly is in contact
    const defReserves = units.filter(u => u.side === "def" && u.parent?.startsWith("RSV/") && !u.routed);
    for (const dr of defReserves) {
        const bn = dr.parent.replace("RSV/", "");
        const fwdBnCos = units.filter(u => u.side === "def" && !u.parent?.startsWith("RSV/") &&
            u.parent === bn && !u.routed);
        const bnUnderPressure = fwdBnCos.some(u => units.some(a => a.side === "atk" && !a.routed && (a.tile === u.tile || ADJ[u.tile].includes(a.tile))));
        if (bnUnderPressure) {
            dr.parent = bn; // commit: strip RSV prefix
            addLog(`${dr.id} (def reserve) committed to reinforce ${bn}`, "info");
        }
    }
}
// -- Tile ownership & fog -----------------------------------------
function updateOwnership() {
    for (let i = 0; i < N_TILES; i++) {
        const atkHere = units.some(u => u.side === "atk" && u.tile === i && !u.routed);
        const defHere = units.some(u => u.side === "def" && u.tile === i && !u.routed);
        if (atkHere && !defHere)
            tiles[i].owner = "atk";
        else if (defHere && !atkHere)
            tiles[i].owner = "def";
        // contested or empty: keep last owner (front line stays where it was)
    }
}
function updateFog() {
    // Attacker fog
    for (let i = 0; i < N_TILES; i++) {
        if (tiles[i].owner === "atk") {
            knownToAtk[i] = true;
            continue;
        }
        if (units.some(u => u.side === "atk" && !u.routed && (u.tile === i || ADJ[u.tile].includes(i)))) {
            knownToAtk[i] = true;
        }
    }
    // Defender fog
    for (let i = 0; i < N_TILES; i++) {
        if (tiles[i].owner === "def") {
            knownToDef[i] = true;
            continue;
        }
        if (units.some(u => u.side === "def" && !u.routed && (u.tile === i || ADJ[u.tile].includes(i)))) {
            knownToDef[i] = true;
        }
    }
    // reveal enemy units on known tiles (attacker spotting)
    for (const d of units.filter(u => u.side === "def" && !u.routed && !u.revealed)) {
        if (knownToAtk[d.tile]) {
            d.revealed = true;
            const ent = d.entrenched ? " [ENTRENCHED]" : "";
            addLog(`Recon: ${d.id} spotted at ${LABELS[d.tile]} (~${Math.round(d.men / 50) * 50} men, ${d.mg}MG)${ent}`, "recon");
        }
    }
}
// -- Movement: 2 MP per unit. Friendly = 1 MP, ZOC/unknown = 2 MP --
function inEnemyZOC(tile, side) {
    return ADJ[tile].some(t => units.some(u => u.side !== side && !u.routed && u.tile === t &&
        (side === "def" ? true : u.revealed)));
}
function moveCost(tile, side) {
    const known = side === "atk" ? knownToAtk[tile] : knownToDef[tile];
    const friendly = tiles[tile].owner === side;
    if (friendly && known)
        return 1;
    return 2; // ZOC, neutral, unknown, or enemy territory
}
function doMovement() {
    // --- Defender movement (fallback to assigned objective) ---
    for (const d of units.filter(u => u.side === "def" && !u.routed && u.type === "inf"
        && !u.parent?.startsWith("RSV/"))) {
        if (d.objective !== undefined && d.tile !== d.objective && !d.objectiveComplete) {
            const targetRow = ROW[d.objective];
            if (ROW[d.tile] > targetRow)
                continue;
            // Fallback retreat: ignore stacking so delay units can retreat through main line
            const rearOpts = ADJ[d.tile].filter(t => ROW[t] < ROW[d.tile] &&
                !units.some(u => u.side === "atk" && u.tile === t && !u.routed));
            if (rearOpts.length) {
                const objCol = colOf(d.objective);
                rearOpts.sort((a, b) => Math.abs(colOf(a) - objCol) - Math.abs(colOf(b) - objCol));
                const from = d.tile;
                d.tile = rearOpts[0];
                d.entrenched = false;
                d.activity = "Falling Back";
                addLog(`${d.id} falls back ${LABELS[from]} -> ${LABELS[d.tile]} (ordered)`, "move");
                if (d.tile === d.objective) {
                    d.entrenched = true;
                    d.objectiveComplete = true;
                    d.activity = "Digging In";
                    addLog(`${d.id} digs in at ${LABELS[d.tile]}`, "info");
                }
            }
        }
    }
    // --- Attacker movement ---
    for (const a of units.filter(u => u.side === "atk" && !u.routed)) {
        if (a.type === "support") {
            const parentCos = units.filter(u => u.side === "atk" && u.parent === a.parent && u.type === "inf" && !u.routed);
            if (parentCos.length) {
                const lead = parentCos.reduce((best, u) => ROW[u.tile] < ROW[best.tile] ? u : best, parentCos[0]);
                if (a.tile !== lead.tile) {
                    a.tile = lead.tile;
                    a.activity = "Supporting";
                    addLog(`${a.id} follows ${lead.id} to ${LABELS[a.tile]}`, "move");
                }
            }
            continue;
        }
        if (a.parent?.startsWith("RSV/") && !reserveCommitted) {
            a.activity = "In Reserve";
            continue;
        }
        // Reorganizing: can't move or attack
        if (a.reorgTurns > 0) {
            a.reorgTurns--;
            a.activity = "Reorganizing";
            addLog(`${a.id} reorganizing at ${LABELS[a.tile]} (${a.reorgTurns} turns left)`, "info");
            continue;
        }
        if (a.objectiveComplete && a.objective !== undefined) {
            a.activity = "Holding OBJ";
            continue;
        }
        let mp = 2;
        a.activity = "Advancing";
        while (mp > 0) {
            const friendlyCount = (t) => units.filter(u => u.side === a.side && u.tile === t && !u.routed && u.id !== a.id).length;
            const enemyOn = (t) => units.some(u => u.side !== a.side && u.tile === t && !u.routed);
            const passable = (t) => !enemyOn(t) && friendlyCount(t) < capacityOf(tiles[t].terrain);
            let opts = ADJ[a.tile].filter(t => ROW[t] < ROW[a.tile]).filter(passable);
            if (!opts.length)
                opts = ADJ[a.tile].filter(t => ROW[t] === ROW[a.tile]).filter(passable);
            if (!opts.length)
                break;
            opts.sort((x, y) => {
                const fx = friendlyCount(x), fy = friendlyCount(y);
                if (fx !== fy)
                    return fx - fy;
                return tiles[y].mul - tiles[x].mul;
            });
            const dest = opts[0];
            const cost = moveCost(dest, a.side);
            if (cost > mp)
                break;
            const enteringZOC = inEnemyZOC(dest, a.side);
            // ZOC entry always costs full 2 MP
            const actualCost = enteringZOC ? 2 : cost;
            if (actualCost > mp)
                break;
            mp -= actualCost;
            const from = a.tile;
            a.tile = dest;
            addLog(`${a.id} marches ${LABELS[from]} -> ${LABELS[dest]} (-${actualCost}MP)${enteringZOC ? " (enters ZOC)" : ""}`, "move");
            autoSpot(a);
            if (enteringZOC) {
                a.activity = "Deploying";
                break;
            }
        }
        if (inEnemyZOC(a.tile, a.side)) {
            if (a.activity !== "Deploying")
                a.activity = "In Contact";
        }
    }
}
function autoSpot(u) {
    for (const ti of [u.tile, ...ADJ[u.tile]]) {
        if (u.side === "atk")
            knownToAtk[ti] = true;
        else
            knownToDef[ti] = true;
        for (const e of units.filter(x => x.side !== u.side && x.tile === ti && !x.routed && !x.revealed)) {
            const pen = tiles[ti].terrain === "forest" ? 2 : tiles[ti].terrain === "village" ? 1 : 0;
            const roll = d6() + (ti === u.tile ? 3 : 0);
            if (roll - pen >= 3) {
                e.revealed = true;
                const ent = e.entrenched ? " [ENT]" : "";
                addLog(`${u.id} spots ${e.id} at ${LABELS[ti]}${ent}`, "recon");
            }
            else if (roll === 1 && ti !== u.tile) {
                const cas = rand(2, 6);
                applyCas(u, cas);
                addLog(`${u.id} takes fire from concealed position near ${LABELS[ti]}: -${cas} men`, "combat");
            }
        }
    }
}
// -- Assaults -----------------------------------------------------
function doAssaults() {
    // Concentrate attacks on as many frontline defenders as possible so that
    // every defending bn is engaged simultaneously -- this neutralises the
    // mutual flanking-fire support each defender otherwise provides.
    const plan = new Map();
    const attackers = units.filter(u => u.side === "atk" && !u.routed && u.type === "inf"
        && !(u.parent?.startsWith("RSV/") && !reserveCommitted));
    const defenderTiles = new Set();
    for (const d of units.filter(u => u.side === "def" && !u.routed && u.revealed))
        defenderTiles.add(d.tile);
    // Phase 1: pin -- each revealed defender tile that is adjacent to at least one
    // attacker gets exactly one attacker assigned (pinning it so it can't flank-fire
    // the main effort).
    const unassigned = new Set(attackers.map(a => a.id));
    for (const ti of defenderTiles) {
        const cands = attackers.filter(a => unassigned.has(a.id) && ADJ[a.tile].includes(ti));
        if (!cands.length)
            continue;
        // Pick the attacker closest in CV to the defender (avoid wasting the strongest bn on the smallest target)
        const dCV = units.filter(u => u.side === "def" && u.tile === ti && !u.routed).reduce((s, u) => s + cv(u), 0);
        cands.sort((a, b) => Math.abs(cv(a) - dCV) - Math.abs(cv(b) - dCV));
        const pick = cands[0];
        plan.set(ti, [pick]);
        unassigned.delete(pick.id);
    }
    // Phase 2: main effort -- any remaining attackers pile onto the weakest
    // already-engaged defender for a decisive break-through.
    if (plan.size > 0) {
        const weakest = [...plan.keys()].reduce((x, y) => {
            const s = (ti) => units.filter(u => u.side === "def" && u.tile === ti && !u.routed).reduce((acc, u) => acc + cv(u), 0);
            return s(y) < s(x) ? y : x;
        });
        for (const a of attackers) {
            if (!unassigned.has(a.id))
                continue;
            if (!ADJ[a.tile].includes(weakest))
                continue;
            plan.get(weakest).push(a);
            unassigned.delete(a.id);
        }
    }
    const capturedTiles = new Set();
    if (plan.size > 0) {
        const posBefore = new Map();
        for (const u of units.filter(u => u.side === "atk" && !u.routed))
            posBefore.set(u.id, u.tile);
        const attackedTiles = new Set(plan.keys());
        for (const [ti, atks] of plan) {
            const ds = units.filter(u => u.side === "def" && u.tile === ti && !u.routed);
            if (ds.length > 0)
                resolveCombat(atks, ds, ti, attackedTiles);
        }
        for (const u of units.filter(u => u.side === "atk" && !u.routed)) {
            const prev = posBefore.get(u.id);
            if (prev !== undefined && prev !== u.tile)
                capturedTiles.add(u.tile);
        }
    }
    // Defender counterattacks -- proactive, not just reactive.
    // A defender will sortie from its tile to strike any adjacent attacker
    // whenever it has at least parity (CV >= 0.9 * enemy CV) and decent morale.
    // Priority targets: (1) attackers on captured friendly tiles, (2) attackers
    // adjacent to multiple defenders (concentrated counter-blow).
    const counterCandidates = units.filter(u => u.side === "def" && !u.routed && u.morale >= 50);
    for (const d of counterCandidates) {
        if (units.some(u => u.side === "atk" && u.tile === d.tile && !u.routed))
            continue;
        const adjAtkTiles = ADJ[d.tile].filter(t => units.some(u => u.side === "atk" && u.tile === t && !u.routed));
        if (!adjAtkTiles.length)
            continue;
        // Score each adjacent attacker-occupied tile and pick the best target.
        adjAtkTiles.sort((x, y) => {
            const score = (t) => {
                const onTile = units.filter(u => u.side === "atk" && u.tile === t && !u.routed);
                const eCV = onTile.reduce((s, u) => s + cv(u), 0);
                const friendsAdj = ADJ[t].filter(a => a !== d.tile &&
                    units.some(u => u.side === "def" && u.tile === a && !u.routed)).length;
                const captured = capturedTiles.has(t) ? 5 : 0;
                // Prefer weaker stacks, captured tiles, and concentrated counter-blows.
                return -eCV + friendsAdj * 2 + captured;
            };
            return score(y) - score(x);
        });
        const target = adjAtkTiles[0];
        const enemy = units.filter(u => u.side === "atk" && u.tile === target && !u.routed);
        const eCV = enemy.reduce((s, u) => s + cv(u), 0);
        if (cv(d) / Math.max(1, eCV) < 0.9)
            continue;
        addLog(`${d.id} counterattacks ${enemy.map(e => e.id).join("+")} at ${LABELS[target]}!`, "combat");
        resolveCounterattack(d, enemy, target);
        capturedTiles.delete(target);
    }
    // Defender low-morale retreat (if not in contact)
    for (const d of units.filter(u => u.side === "def" && !u.routed && u.morale < 35)) {
        if (units.some(u => u.side === "atk" && ADJ[d.tile].includes(u.tile) && !u.routed))
            continue;
        const bk = ADJ[d.tile].filter(t => ROW[t] < ROW[d.tile]);
        if (!bk.length)
            continue;
        const from = d.tile;
        d.tile = bk[rand(bk.length)];
        d.entrenched = false;
        addLog(`${d.id} retreats ${LABELS[from]} -> ${LABELS[d.tile]} (M:${d.morale}, lost entrenchment)`, "move");
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
                cas += Math.floor(rand(1, 4) / tiles[ti].mul);
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
        const cas = Math.max(1, Math.floor(rand(1, 3) * defGuns / Math.max(1, tgts.length) / tiles[ti].mul));
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
function resolveCombat(atks, defs, ti, attackedTiles) {
    const tile = tiles[ti];
    let atkCV = atks.reduce((s, a) => s + cv(a), 0);
    const defCV = defs.reduce((s, d) => s + cv(d), 0);
    if (atkCV <= 0 || defCV <= 0)
        return;
    // Forest: effective attacker CV is halved (dense terrain limits combat power)
    if (tile.terrain === "forest") {
        atkCV = Math.floor(atkCV / 2);
        addLog(`  forest reduces attacker effective CV to ${atkCV}`, "combat");
    }
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
    // Linear scaling: defender casualties scale with force ratio
    // Base rates at 1:1, then dr scales linearly with eff
    const baseAr = 0.03;
    const baseDr = 0.012;
    if (eff >= 1.0) {
        ar = Math.max(0.005, baseAr / eff); // attacker losses decrease with advantage
        dr = Math.min(0.06, baseDr * eff); // defender losses increase linearly
    }
    else {
        ar = Math.min(0.06, baseAr / eff); // attacker takes more at disadvantage
        dr = Math.max(0.004, baseDr * eff); // defender takes less
    }
    // Delay stance: defenders trading space for time — both sides take fewer casualties
    const isDelay = defs.some(d => d.stance === "delay");
    if (isDelay) {
        ar *= 0.5; // attacker takes half casualties (defenders pulling back, not fully committed)
        dr *= 0.4; // defender takes even less (fighting withdrawal, not holding to the last)
        addLog(`  delay action: reduced casualties for both sides`, "combat");
        // Track engagement turns for delay units
        for (const d of defs.filter(d => d.stance === "delay")) {
            d.delayTurns = (d.delayTurns ?? 0) + 1;
        }
    }
    // Forest: overall casualties reduced (units spread out, harder to hit)
    const casualtyMod = tile.terrain === "forest" ? 0.6 : 1.0;
    const aLoss = Math.max(1, Math.floor(tAmen * ar * (0.7 + Math.random() * 0.6) * casualtyMod));
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
        const l = Math.max(1, Math.floor(d.men * dr * (0.7 + Math.random() * 0.6) * casualtyMod));
        applyCas(d, l);
        d.morale = clamp(d.morale - Math.ceil(l / 8), 0, 100);
        dLoss += l;
        // Delay stance: orderly retreat after 2 turns of engagement
        if (d.stance === "delay" && (d.delayTurns ?? 0) >= 2) {
            const rear = ADJ[d.tile].filter(t => ROW[t] < ROW[d.tile] &&
                !units.some(u2 => u2.side === "atk" && u2.tile === t && !u2.routed));
            if (rear.length) {
                const from = d.tile;
                d.tile = rear[rand(rear.length)];
                d.entrenched = false;
                d.delayTurns = 0;
                addLog(`  ${d.id} completes delay — withdraws ${LABELS[from]} -> ${LABELS[d.tile]} (good order)`, "move");
                continue;
            }
        }
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
    const dHold = defs.filter(d => !d.routed && d.tile === ti);
    let out;
    if (!dHold.length && alive.length) {
        for (const a of alive) {
            a.tile = ti;
            a.reorgTurns = 1; // must reorganize after assault
            a.activity = "Reorganizing";
            for (const r of units.filter(u => u.type === "recon" && u.parent === a.id && !u.routed))
                r.tile = ti;
        }
        out = "TAKES " + LABELS[ti];
    }
    else if (alive.length) {
        // Set activity based on assault intensity
        for (const a of alive)
            a.activity = "Engaging";
        for (const d of dHold)
            d.activity = d.stance === "delay" ? "Delaying" : "Defending";
        if (eff >= 2.0)
            out = "hammers " + LABELS[ti];
        else if (eff >= 1.4)
            out = "presses " + LABELS[ti];
        else
            out = "repelled from " + LABELS[ti];
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
        cr = 0.008;
        er = 0.03;
    }
    else if (eff >= 1.2) {
        cr = 0.015;
        er = 0.015;
    }
    else if (eff >= 0.8) {
        cr = 0.025;
        er = 0.01;
    }
    else {
        cr = 0.035;
        er = 0.006;
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
        // Try to push every surviving attacker off the tile. Only move the counter-attacker
        // onto the tile if every attacker actually had somewhere to retreat to -- otherwise
        // we'd end up with friendly + enemy units co-located on the same hex.
        const pushed = [];
        for (const e of eAlive) {
            const back = ADJ[e.tile].filter(t => ROW[t] > ROW[e.tile] &&
                !units.some(u2 => u2.side === "def" && u2.tile === t && !u2.routed));
            if (back.length) {
                e.tile = back[rand(back.length)];
                pushed.push(e);
            }
        }
        if (pushed.length === eAlive.length) {
            ctr.tile = ti;
            ctr.entrenched = false;
            result = `drives enemy from ${LABELS[ti]}`;
        }
        else {
            result = `pushes ${LABELS[ti]} but enemy holds`;
        }
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
    // Victory requires ALL attacker order sets completed (all 3 rows seized)
    const allOrdersDone = orderSet >= 3 && units.filter(u => u.side === "atk" && u.type === "inf" && !u.routed
        && u.objective !== undefined).every(u => u.objectiveComplete);
    const defAlive = units.filter(u => u.side === "def" && !u.routed);
    const atkAlive = units.filter(u => u.side === "atk" && !u.routed && u.type === "inf");
    if (allOrdersDone) {
        over = true;
        addLog(`\n* ATTACKER VICTORY -- all orders completed, defence broken`, "result");
    }
    else if (!defAlive.length) {
        over = true;
        addLog("\n* ATTACKER VICTORY -- all defenders routed", "result");
    }
    else if (!atkAlive.length) {
        over = true;
        addLog("\n* DEFENDER VICTORY -- all attackers routed", "result");
    }
    else if (turn >= 25) {
        over = true;
        addLog(`\n* STALEMATE -- nightfall (${tStr(turn)})`, "result");
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
    renderTurnLog(f);
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
    // hexes
    for (let i = 0; i < N_TILES; i++) {
        const p = HP[i], t = tiles[i];
        const onTile = f.units.filter(u => u.tile === i && !u.routed);
        const hasAtk = onTile.some(u => u.side === "atk");
        const hasDef = onTile.some(u => u.side === "def" && u.revealed);
        const contested = hasAtk && hasDef;
        const owner = f.owners[i];
        const known = viewSide === "atk" ? f.known[i] : f.knownDef[i];
        const ownerCls = owner === "atk" ? " own-atk" : owner === "def" ? " own-def" : " own-neu";
        // Objective markers: show only for the viewed side
        const objCos = f.units.filter(u => u.objective === i && !u.routed && u.side === viewSide);
        const objDone = objCos.length > 0 && objCos.every(u => u.objectiveComplete);
        const objMark = objCos.length > 0
            ? `<div class="hex-obj${objDone ? " obj-done" : ""}">${objCos.map(u => u.id.split("/")[0] + "/" + u.id.split("/")[1]).join(" ")}</div>`
            : "";
        html += `<div class="hex-wrap" data-tile="${i}" style="left:${p.x}px;top:${p.y}px;width:${HW}px;height:${HH}px;cursor:pointer">`
            + `<div class="hex-bdr${contested ? " contested" : ""}${ownerCls}" style="clip-path:${CLIP}"></div>`
            + `<div class="hex-fill ter-${t.terrain}" style="clip-path:${CLIP}"></div>`
            + `<div class="hex-info" style="clip-path:${CLIP}">`
            + `<span class="hex-id">${LABELS[i]}</span>`
            + `<span class="hex-ter">${esc(t.terrain)} x${t.mul}</span>`
            + `</div>`
            + objMark
            + `<div class="hex-units">${mkUnits(onTile, f.reconOut, known)}</div>`
            + `</div>`;
    }
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
    const mapBottom = MAP_PAD_TOP + HO + (ROWS - 1) * RS + HH;
    html += `<div class="map-scale" style="top:${mapBottom + 10}px">1 hex ~ 1 km across</div>`;
    // off-map attacker art (below the grid)
    const botY = mapBottom + 14;
    html += `<div class="offmap offmap-atk" style="top:${botY}px">`
        + `<div class="offmap-sym">${symSvg(SIDC_ATK_ART, 24)}</div>`
        + `<div class="offmap-info"><div class="offmap-nm">394th Art Bn</div>`
        + `<div class="offmap-det">${f.atkGuns}x 105mm | ${f.atkGuns > 0 ? "READY" : "DESTROYED"}</div></div></div>`;
    g.innerHTML = html;
    // Click-to-inspect: delegate from grid container
    g.querySelectorAll(".hex-wrap").forEach((hw) => {
        hw.addEventListener("click", () => renderTileInfo(Number(hw.dataset.tile), steps[stepIdx]));
    });
}
function renderTileInfo(ti, f) {
    const panel = $("tile-panel");
    const t = tiles[ti];
    const known = viewSide === "atk" ? f.known[ti] : f.knownDef[ti];
    const owner = f.owners[ti];
    const ownerLabel = owner === "atk" ? "Attacker" : owner === "def" ? "Defender" : "Neutral";
    const onTile = f.units.filter(u => u.tile === ti);
    const mySide = viewSide;
    const visibleUnits = onTile.filter(u => !u.routed && (u.side === mySide || known));
    const routedUnits = onTile.filter(u => u.routed && (u.side === mySide || known));
    $("tp-title").textContent = `Hex ${LABELS[ti]}`;
    let body = `<span class="tp-owner ${owner === "atk" ? "own-atk" : owner === "def" ? "own-def" : "own-neu"}">${ownerLabel}</span>`;
    body += `<div class="tp-terrain">`
        + `<b>${t.terrain.charAt(0).toUpperCase() + t.terrain.slice(1)}</b> &nbsp;·&nbsp; Defence ×${t.mul}`
        + `</div>`;
    const renderUnitBlock = (u, dim = false) => {
        const nameCls = u.routed ? " routed" : "";
        const sup = ["—", "Disrupted", "Suppressed", "Pinned"][u.suppression] ?? "—";
        const stanceStr = u.stance ? ` [${u.stance.toUpperCase()}]` : "";
        const status = u.routed ? "ROUTED"
            : u.entrenched ? `Entrenched${stanceStr}`
                : u.suppression > 0 ? `${sup}${stanceStr}`
                    : `Ready${stanceStr}`;
        const actLabel = u.activity ? `<div class="tp-unit-act">${u.activity}</div>` : "";
        const sidcSvg = symSvg(u.sidc, 28);
        return `<div class="tp-unit" style="${dim ? "opacity:.45" : ""}">`
            + `<div class="tp-unit-sym">${sidcSvg}</div>`
            + `<div class="tp-unit-body">`
            + `<div class="tp-unit-name${nameCls}">${esc(u.id)}</div>`
            + `<div class="tp-unit-stats">Men: <b>${u.men}</b> &nbsp;MG: <b>${u.mg}</b> &nbsp;Mor: <b>${u.mortar}</b> &nbsp;AT: <b>${u.at}</b> &nbsp;CV: <b>${cv(u).toFixed(1)}</b></div>`
            + `<div class="tp-unit-stats">Morale: <b>${u.morale}</b></div>`
            + `<div class="tp-unit-status">${status}</div>`
            + actLabel
            + `</div></div>`;
    };
    if (!visibleUnits.length && !routedUnits.length) {
        body += `<div class="tp-fog">No units</div>`;
    }
    else {
        if (visibleUnits.length) {
            body += `<div class="tp-section">Units present</div>`;
            body += visibleUnits.map(u => renderUnitBlock(u)).join("");
        }
        if (routedUnits.length) {
            body += `<div class="tp-section">Routed (withdrawing)</div>`;
            body += routedUnits.map(u => renderUnitBlock(u, true)).join("");
        }
    }
    const adjUnits = [];
    for (const adj of ADJ[ti]) {
        const adjKnown = viewSide === "atk" ? f.known[adj] : f.knownDef[adj];
        for (const u of f.units.filter(u => u.tile === adj && !u.routed && (u.side === viewSide || adjKnown))) {
            adjUnits.push(u);
        }
    }
    if (adjUnits.length) {
        body += `<div class="tp-section">Adjacent (${adjUnits.length})</div>`;
        body += adjUnits.map(u => `<div style="font-size:9px;color:#555;padding:1px 0">${esc(u.id)} @ ${LABELS[u.tile]}</div>`).join("");
    }
    $("tp-body").innerHTML = body;
    panel.classList.add("open");
}
function mkUnits(on, reconOut, tileKnown) {
    // Show own units always, show enemy only on known tiles
    const visible = on.filter(u => !u.routed && (u.side === viewSide || tileKnown));
    const sorted = [...visible].sort((a, b) => {
        if (a.side !== b.side)
            return a.side === "def" ? -1 : 1;
        return 0;
    });
    // counter-game stacking: each successive unit cascades down-right on top of the previous one
    return sorted.map((u, i) => mkUnit(u, 10 + i, i)).join("");
}
function mkUnit(u, z, stack = 0) {
    const fog = !u.revealed && u.side !== viewSide;
    const sup = u.suppression > 0 ? ` u-s${u.suppression}` : "";
    const cls = u.side === "atk" ? "u-atk" : "u-def";
    const szPx = u.size === "bn" ? 16 : u.size === "co" ? 14 : 12;
    if (fog) {
        return `<div class="unit ${cls} u-fog" style="z-index:${z};--sx:${stack}" title="Unidentified enemy unit">`
            + `<div class="u-sym">${symSvg(SIDC_UNK, 16)}</div>`
            + `<div class="u-label">???</div></div>`;
    }
    const ent = u.entrenched ? " ENT" : "";
    const st = u.suppression > 0 ? " " + SUPP[u.suppression].substring(0, 4) : "";
    const act = u.activity ? ` [${u.activity}]` : "";
    const tip = `${u.id}  ${u.men} men  CV:${cv(u)}  ${u.mg}MG ${u.mortar}Mor ${u.at}AT  M:${u.morale}  ${SUPP[u.suppression]}${u.entrenched ? "  ENTRENCHED" : ""}  ${u.activity}`;
    const rcn = u.type === "recon";
    return `<div class="unit ${cls}${sup}${rcn ? " u-rcn" : ""}" style="z-index:${z};--sx:${stack}" title="${esc(tip)}">`
        + `<div class="u-sym">${symSvg(u.sidc, szPx)}</div>`
        + `<div class="u-label"><div class="u-name">${u.id}</div>${u.men} CV:${cv(u)}${ent}${st}<div class="u-act">${u.activity || ""}</div></div>`
        + `</div>`;
}
// -- OOB tables ---------------------------------------------------
function renderOOB(fu) {
    // Attacker bns: show details only if viewSide is atk
    let aHtml = "";
    for (const bn of fu.filter(u => u.side === "atk"))
        aHtml += oobRow(bn, viewSide === "atk" || bn.revealed || bn.routed);
    $("oob-a").innerHTML = aHtml;
    // Defender: show details only if viewSide is def
    $("oob-d").innerHTML = fu.filter(u => u.side === "def")
        .map(u => oobRow(u, viewSide === "def" || u.revealed || u.routed)).join("");
}
function oobRow(u, known) {
    if (!known)
        return `<tr class="oob-tr fog"><td class="oob-bn">${esc(u.id)}</td><td colspan="6" class="oob-unk">???</td></tr>`;
    const cls = u.routed ? "oob-tr rt" : "oob-tr";
    const sup = u.suppression > 0 && !u.routed ? ` s${u.suppression}` : "";
    const actShort = u.activity ? u.activity.substring(0, 8) : "";
    return `<tr class="${cls}${sup}">`
        + `<td class="oob-bn">${esc(u.id)}${u.entrenched ? " E" : ""}${u.stance === "delay" ? " D" : ""}</td>`
        + `<td class="oob-n">${u.routed ? "--" : u.men}</td>`
        + `<td class="oob-n oob-cv">${cv(u)}</td>`
        + `<td class="oob-n">${u.morale}</td>`
        + `<td class="oob-h">${LABELS[u.tile]}</td>`
        + `<td class="oob-act">${actShort}</td>`
        + `<td class="oob-h">${u.objective !== undefined ? (u.objectiveComplete ? "\u2713" : LABELS[u.objective]) : ""}</td>`
        + `</tr>`;
}
// -- Log ----------------------------------------------------------
function renderLog(logEnd) {
    const el = $("log");
    el.innerHTML = "";
    // Show only entries for the current turn (between previous and current step)
    const prevEnd = stepIdx > 0 ? steps[stepIdx - 1].logEnd : 0;
    const entries = fullLog.slice(prevEnd, logEnd);
    for (const e of entries) {
        const d = document.createElement("div");
        d.className = "l-" + e.type;
        d.textContent = e.text;
        el.appendChild(d);
    }
    el.scrollTop = el.scrollHeight;
}
function renderTurnLog(f) {
    const el = $("turn-log");
    const prevEnd = stepIdx > 0 ? steps[stepIdx - 1].logEnd : 0;
    const entries = fullLog.slice(prevEnd, f.logEnd)
        .filter(e => e.type === "combat" || e.type === "result" || e.type === "arty");
    if (!entries.length) {
        el.innerHTML = '<div class="tl-empty">No combat this turn</div>';
        return;
    }
    let html = '';
    for (const e of entries)
        html += `<div class="l-${e.type}">${esc(e.text)}</div>`;
    el.innerHTML = html;
}
// -- Status / controls --------------------------------------------
function renderPhaseBar(f) {
    const orderLabel = f.orderSet === 1 ? "Seize Row D" : f.orderSet === 2 ? "Seize Row C" : "Seize Row B";
    const defLabel = f.defOrderSet === 1 ? "Hold C+D" : f.defOrderSet === 2 ? "Hold Row C" : "Hold Row B";
    const label = f.over ? "RESULT" : `${tStr(f.turn)}  |  ATK: ${orderLabel}  |  DEF: ${defLabel}`;
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
    const minutes = (fs.turn - 1) * 30;
    const dur = `${Math.floor(minutes / 60)}h ${minutes % 60}m`;
    h += `<div class="res-sub">${tStr(1)}–${tStr(fs.turn)} (${dur})  |  Atk Art: ${fs.atkGuns}/${steps[0].atkGuns} guns  |  Def Art: ${fs.defGuns}/${steps[0].defGuns} guns</div>`;
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
            const typeStr = iu.type === "recon" ? "RCN" : iu.type === "support" ? "SPT" : "INF";
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
$("tp-close").addEventListener("click", () => $("tile-panel").classList.remove("open"));
$("reset").addEventListener("click", initBattle);
$("view-toggle").addEventListener("click", () => {
    viewSide = viewSide === "atk" ? "def" : "atk";
    const btn = $("view-toggle");
    btn.textContent = viewSide === "atk" ? "VIEW: BLUE" : "VIEW: RED";
    btn.className = viewSide === "atk" ? "view-btn view-atk" : "view-btn view-def";
    renderFrame();
});
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
        case "v":
            $("view-toggle").click();
            e.preventDefault();
            break;
    }
});
initBattle();
