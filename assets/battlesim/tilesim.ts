// =================================================================
//  BATTLES -- Regiment Combat on a 5-deep Hex Grid
// =================================================================
//  milsymbol NATO counters, recon detachments, off-map artillery,
//  flanking fire, entrenchment.  No emojis -- text only.
// =================================================================

declare namespace ms {
  class Symbol {
    constructor(sidc: string, options?: {
      size?: number;
      uniqueDesignation?: string;
      higherFormation?: string;
      additionalInformation?: string;
      staffComments?: string;
      type?: string;
      [key: string]: unknown;
    });
    asSVG(): string;
    getSize(): { width: number; height: number };
  }
}

// -- Types --------------------------------------------------------

type UnitType = "inf" | "recon" | "support";
type UnitSize = "bn" | "co" | "plt";

interface Unit {
  id: string;       // "I/394" or "I/394/R"
  name: string;     // "I" or "I/R"
  rgt: string;      // "394th"
  side: "atk"|"def";
  type: UnitType;
  size: UnitSize;
  men: number;
  rifles: number;
  mg: number;
  mortar: number;
  at: number;       // anti-tank guns (used as direct fire support)
  morale: number;
  suppression: number;
  tile: number;
  revealed: boolean;
  routed: boolean;
  entrenched: boolean;
  sidc: string;
  parent?: string;
}

interface Tile { index: number; terrain: string; mul: number; owner: "atk"|"def"|"neutral"; }
interface LogEntry { text: string; type: "hdr"|"info"|"recon"|"arty"|"combat"|"move"|"result"; }
interface Frame {
  turn: number; reconOut: boolean; units: Unit[];
  owners: ("atk"|"def"|"neutral")[];
  known: boolean[]; // tile is known to attacker (revealed enemies / fog cleared)
  atkGuns: number; defGuns: number;
  logEnd: number; over: boolean;
}

// -- Grid constants -----------------------------------------------
// 4 columns × 10 rows = 40 tiles, flat-top hexes ("odd-q" offset: odd cols shifted down)
// Row A = top (deep defender rear), Row J = bottom (deep attacker rear)
// Maneuver warfare: full contact from turn 1 — defenders on rows B-C, attackers G-H.

const COLS = 4, ROWS = 10;
const N_TILES = COLS * ROWS;
const ROW_NAMES = ["A", "B", "C", "D", "E", "F", "G", "H", "I", "J"];

const LABELS: string[] = [];
for (let r = 0; r < ROWS; r++) {
  for (let c = 0; c < COLS; c++) LABELS.push(ROW_NAMES[r] + (c + 1));
}

function tileIdx(c: number, r: number): number { return r * COLS + c; }
function colOf(i: number): number { return i % COLS; }

const ROW: number[] = [];
for (let i = 0; i < N_TILES; i++) ROW.push(Math.floor(i / COLS));

// flat-top "odd-q" offset neighbour formula
const ADJ: number[][] = [];
for (let i = 0; i < N_TILES; i++) {
  const c = colOf(i), r = ROW[i];
  const odd = (c % 2) === 1;  // odd columns are offset DOWN by half a hex
  const nbrs: [number, number][] = odd
    ? [[c, r - 1], [c, r + 1], [c - 1, r], [c - 1, r + 1], [c + 1, r], [c + 1, r + 1]]
    : [[c, r - 1], [c, r + 1], [c - 1, r - 1], [c - 1, r], [c + 1, r - 1], [c + 1, r]];
  ADJ.push(
    nbrs.filter(([nc, nr]) => nc >= 0 && nc < COLS && nr >= 0 && nr < ROWS)
        .map(([nc, nr]) => tileIdx(nc, nr))
  );
}

// Zone labels — maneuver warfare (everything under attack from turn 1)
const ZONES = [
  "DEFENSE IN DEPTH",  // A — deep defender rear
  "MAIN DEFENSE LINE", // B — defender front
  "FORWARD LINE",      // C — defender forward screen
  "BREACH",            // D — attacker already pushing
  "EXPLOITATION",      // E
  "EXPLOITATION",      // F
  "ATTACK ASSEMBLY",   // G — attacker front
  "ASSAULT LINE",      // H — attacker main body
  "SUPPORT",           // I — attacker support
  "ATTACKER REAR",     // J
];

const TERRAIN_DATA: {name:string; mul:number}[] = [
  { name:"field",  mul:1.0 },
  { name:"hill",   mul:1.5 },
  { name:"village",mul:1.8 },
  { name:"forest", mul:2.0 },
  { name:"ridge",  mul:1.7 },
];
const SUPP = ["READY","DISRUPTED","SUPPRESSED","PINNED"];

// SIDC codes (15-char, MIL-STD-2525C). Position 12 = echelon; "E" = company.
const SIDC_ATK_INF  = "SFGPUCI----E---";   // friendly infantry company
const SIDC_DEF_INF  = "SHGPUCI----E---";   // hostile infantry company
const SIDC_ATK_RCN  = "SFGPUCR----D---";   // friendly recon plt
const SIDC_ATK_SPT  = "SFGPUCI----D---";   // friendly support plt
const SIDC_ATK_ART  = "SFGPUCF----E---";   // friendly arty battery
const SIDC_DEF_ART  = "SHGPUCF----E---";   // hostile arty battery
const SIDC_UNK      = "SHGPUCI----E---";   // unknown hostile (shown foggy)

// hex layout — flat-top orientation (flat edges at top/bottom, vertices left/right)
// HW = vertex-to-vertex width = 2s, HH = flat-to-flat height = s√3
const HW = 120, HH = 104;
const CS = Math.floor(HW * 0.75) + 4;  // column-to-column spacing (3s/2 + gap)
const RS = HH + 4;                      // row-to-row spacing within a column
const HO = Math.floor(RS / 2);          // vertical offset for 3-tile (odd) columns
const CLIP = "polygon(25% 0%,75% 0%,100% 50%,75% 100%,25% 100%,0% 50%)";
const MAP_PAD_TOP = 40;
const MAP_PAD_LEFT = 130;   // wide left margin so row-letter zone labels fit

const HP: {x:number;y:number}[] = [];
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

let tiles: Tile[] = [];
let units: Unit[] = [];
let fullLog: LogEntry[] = [];
let turn = 0, over = false;
let atkGuns = 12, defGuns = 8;
let steps: Frame[] = [];
let stepIdx = 0;
let knownToAtk: boolean[] = []; // per-tile fog state for attacker side
let reserveCommitted = false;

// -- Helpers ------------------------------------------------------

function rand(a: number, b?: number): number {
  if (b === undefined) return Math.floor(Math.random() * a);
  return a + Math.floor(Math.random() * (b - a + 1));
}
function d6(): number { return rand(1, 6); }
function clamp(v: number, lo: number, hi: number): number { return Math.max(lo, Math.min(hi, v)); }
// 15-minute turns. Turn 1 = 14:00, turn 2 = 14:15, ...
function tStr(t: number): string {
  const totalMin = 14 * 60 + (t - 1) * 15;
  const hh = Math.floor(totalMin / 60), mm = totalMin % 60;
  return String(hh).padStart(2, '0') + String(mm).padStart(2, '0');
}
function $(id: string): HTMLElement { return document.getElementById(id)!; }
function esc(s: string): string { const el = document.createElement("span"); el.textContent = s; return el.innerHTML; }
function addLog(t: string, ty: LogEntry["type"] = "info"): void { fullLog.push({ text: t, type: ty }); }

function cv(u: Unit): number {
  return Math.round(u.rifles / 10 + u.mg + u.mortar * 0.67 + u.at * 1.5);
}

function applyCas(u: Unit, n: number): void {
  const actual = Math.min(n, Math.max(0, u.men));
  u.men -= actual;
  u.rifles = Math.max(0, u.rifles - Math.floor(actual * 0.85));
  if (actual >= 25) u.mg = Math.max(0, u.mg - Math.floor(actual / 70));
  if (actual >= 50 && u.mortar > 0 && d6() <= 2) u.mortar--;
  if (actual >= 40 && u.at > 0 && d6() <= 2) u.at--;
}

function snap(reconOut = false): Frame {
  return {
    turn, reconOut,
    units: units.map(u => ({ ...u })),
    owners: tiles.map(t => t.owner),
    known: knownToAtk.slice(),
    atkGuns, defGuns, logEnd: fullLog.length, over,
  };
}

// milsymbol SVG cache
const _svgCache = new Map<string, string>();
function symSvg(sidc: string, size: number): string {
  const key = sidc + ":" + size;
  let s = _svgCache.get(key);
  if (!s) {
    if (typeof ms !== "undefined") {
      s = new ms.Symbol(sidc, { size, frame: true, fill: true, strokeWidth: 3 }).asSVG();
    } else {
      s = `<span style="font-size:${size}px;color:#888">[${sidc.substring(4,7)}]</span>`;
    }
    _svgCache.set(key, s);
  }
  return s;
}

// -- Init & Simulate ----------------------------------------------

// Stacking capacity by terrain: forest/ridge can hide 2 companies, others only 1.
function capacityOf(terrain: string): number {
  return (terrain === "forest" || terrain === "ridge") ? 2 : 1;
}

function initBattle(): void {
  _svgCache.clear();

  // Generate terrain per row (COLS=4 entries each).
  // Defender half (rows 0-2): strong cover; attacker half (rows 7-9): open ground.
  const TERR_DEF = ["forest", "ridge", "village", "hill"];
  const TERR_MID = ["field", "hill", "forest", "field"];
  const TERR_ATK = ["field", "field", "hill", "field"];
  const pool: string[] = [];
  for (let r = 0; r < ROWS; r++) {
    const tab = r <= 2 ? TERR_DEF : r >= 7 ? TERR_ATK : TERR_MID;
    const shuf = tab.slice();
    for (let i = shuf.length - 1; i > 0; i--) {
      const j = rand(i + 1);[shuf[i], shuf[j]] = [shuf[j], shuf[i]];
    }
    for (let c = 0; c < COLS; c++) pool.push(shuf[c % shuf.length]);
  }

  tiles = pool.map((t, i) => {
    const info = TERRAIN_DATA.find(x => x.name === t)!;
    const owner: "atk"|"def"|"neutral" =
      ROW[i] >= 7 ? "atk" : ROW[i] <= 2 ? "def" : "neutral";
    return { index: i, terrain: t, mul: info.mul, owner };
  });

  units = [];

  // --- Deploy defenders ---
  // RULE: Every tile in row B (index 1) = the MAIN DEFENSE LINE must have exactly 1 co.
  // With COLS=4 that's 4 tiles × 1 co = 4 cos. No tile can be left empty or attackers walk through.
  // The remaining 2 forward cos go to the 2 best-cover tiles in row C (forward screen).
  // Reserve: 1 co per bn on row A (deep rear / defence in depth).

  const rowBTiles = [...Array(N_TILES).keys()]
    .filter(i => ROW[i] === 1);   // always exactly COLS=4 tiles → full blocking line
  const rowCTiles = [...Array(N_TILES).keys()]
    .filter(i => ROW[i] === 2)
    .sort((a, b) => tiles[b].mul - tiles[a].mul);  // best cover first for forward screen
  const rowATiles = [...Array(N_TILES).keys()]
    .filter(i => ROW[i] === 0);

  // 6 forward cos: 4 on row B (one per tile, guaranteed full line) + 2 on row C
  const defFront: number[] = [
    ...rowBTiles,                   // 4 cos — one per tile — full blocking line
    rowCTiles[0],                   // 5th co on best-cover row C tile
    rowCTiles[Math.min(1, rowCTiles.length - 1)],  // 6th co
  ];

  const defRear: number[] = rowATiles.slice(0, 3);
  while (defRear.length < 3) defRear.push(rowATiles[0]);

  const defBnNames = ["I", "II", "III"];
  let di = 0;
  for (let bi = 0; bi < defBnNames.length; bi++) {
    const bn = defBnNames[bi];
    // cos 1+2 = front line (row B + C), co 3 = reserve on row A
    for (let n = 1; n <= 3; n++) {
      const tile = n <= 2 ? defFront[di++] : defRear[bi];
      units.push(mkCo(bn, n, "1028th", "def",
        220, 180, 3, 2, 1, 75, tile));
    }
  }

  // --- Deploy attackers ---
  // 2 assault cos per bn on rows 6-7 (G-H, close to enemy).
  // 1 reserve co per bn on row 8 (I, support/follow-on).
  // Support platoon (arty/mortars) placed with the LEADING assault co of I Bn.
  const atkAssaultRows = [6, 7];
  const atkAssaultCands = [...Array(N_TILES).keys()]
    .filter(i => atkAssaultRows.includes(ROW[i]))
    .sort((a, b) => tiles[b].mul - tiles[a].mul);
  const atkAssault: number[] = [];  // 6 assault cos
  for (const t of atkAssaultCands) {
    if (atkAssault.length >= 6) break;
    const cap = capacityOf(tiles[t].terrain);
    for (let k = 0; k < cap && atkAssault.length < 6; k++) atkAssault.push(t);
  }
  while (atkAssault.length < 6) atkAssault.push(atkAssaultCands[0]);

  const atkRearCands = [...Array(N_TILES).keys()]
    .filter(i => ROW[i] === 8)
    .sort((a, b) => tiles[a].mul - tiles[b].mul); // open ground for reserve
  const atkRear: number[] = [];
  for (const t of atkRearCands) { if (atkRear.length >= 3) break; atkRear.push(t); }
  while (atkRear.length < 3) atkRear.push(atkRearCands[0]);

  const atkBnNames = ["I", "II", "III"];
  let ai = 0;
  for (let bi = 0; bi < atkBnNames.length; bi++) {
    const bn = atkBnNames[bi];
    for (let n = 1; n <= 3; n++) {
      const tile = n <= 2 ? atkAssault[ai++] : atkRear[bi];
      units.push(mkCo(bn, n, "394th", "atk",
        220, 180, 3, 2, 0, 80, tile));
    }
  }

  // Support platoon: arty/mortar element co-located with leading I Bn co.
  // Represented as a separate unit with higher mortar/at but fewer rifles.
  const leadTile = atkAssault[0];
  units.push({
    id: "SPT/394", name: "SPT", rgt: "394th", side: "atk", type: "support", size: "plt",
    men: 60, rifles: 30, mg: 1, mortar: 4, at: 2, morale: 85,
    suppression: 0, tile: leadTile, revealed: true, routed: false, entrenched: false,
    sidc: SIDC_ATK_SPT, parent: "I/394",
  });

  reserveCommitted = false;
  knownToAtk = tiles.map(() => false);
  updateOwnership();
  updateFog();

  atkGuns = 12; defGuns = 8;
  turn = 1; over = false;
  fullLog = [];
  simulateAll();
  stepIdx = 0;
  renderFrame();
}

function mkCo(bn: string, num: number, rgt: string, side: "atk"|"def",
  men: number, rifles: number, mg: number, mortar: number, at: number,
  morale: number, tile: number): Unit {
  const name = num + "/" + bn;                       // e.g. "1/I"
  const rgtShort = rgt.replace("th", "");
  // Co 3 of each bn is the reserve for that bn.
  const isReserve = num === 3;
  return {
    id: name + "/" + rgtShort,                       // e.g. "1/I/394"
    name, rgt, side, type: "inf", size: "co",
    men, rifles, mg, mortar, at, morale,
    suppression: 0, tile, revealed: side === "atk",
    routed: false, entrenched: side === "def",
    sidc: side === "atk" ? SIDC_ATK_INF : SIDC_DEF_INF,
    parent: (isReserve ? "RSV/" : "") + bn + "/" + rgtShort,  // RSV/I/394 = reserve
  };
}

function simulateAll(): void {
  steps = [];
  const atk = units.filter(u => u.side === "atk");
  const def = units.filter(u => u.side === "def");
  addLog("== 394th Infantry Rgt  vs  1028th Rifle Rgt ==", "hdr");
  addLog(`394th: 9 cos (2 assault + 1 reserve per bn) + SPT plt  CV:${atk.reduce((s,u)=>s+cv(u),0)}`, "info");
  addLog(`1028th: 9 cos (2 front + 1 reserve per bn)  CV:${def.reduce((s,u)=>s+cv(u),0)}  ENTRENCHED`, "info");
  addLog("Maneuver: full contact from turn 1. Reserves commit when bn is under pressure.", "info");
  steps.push(snap());

  while (!over) {
    turn++;
    for (const u of units) if (!u.routed) u.suppression = Math.max(0, u.suppression - 1);
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

    checkEnd();
    steps.push(snap());
  }
}

// -- Reserve ------------------------------------------------------

function checkReserveCommit(): void {
  if (reserveCommitted) return;
  // Attacker reserves: cos 3 of each bn (parent starts with "RSV/")
  const reserve = units.filter(u => u.side === "atk" && u.parent?.startsWith("RSV/") && !u.routed);
  if (!reserve.length) { reserveCommitted = true; return; }

  const fwdCos = units.filter(u => u.side === "atk" && !u.parent?.startsWith("RSV/") && u.type === "inf");
  const routedCount = fwdCos.filter(u => u.routed).length;
  const allEngaged = fwdCos.filter(u => !u.routed).every(u =>
    units.some(d => d.side === "def" && !d.routed && (d.tile === u.tile || ADJ[u.tile].includes(d.tile))));
  const lateGame = turn >= 12;  // 12 × 15 min = 3 h elapsed

  if (routedCount >= 2 || (allEngaged && turn >= 6) || lateGame) {
    reserveCommitted = true;
    // Un-tag reserves so they behave like normal cos and follow the main force
    for (const r of reserve) r.parent = r.parent!.replace("RSV/", "");
    addLog(`Reserves committed! 3rd cos of I/II/III bn advancing.`, "info");
  }

  // Defender reserve: co 3 of each bn commits when adjacent friendly is in contact
  const defReserves = units.filter(u => u.side === "def" && u.parent?.startsWith("RSV/") && !u.routed);
  for (const dr of defReserves) {
    const bn = dr.parent!.replace("RSV/", "");
    const fwdBnCos = units.filter(u => u.side === "def" && !u.parent?.startsWith("RSV/") &&
      u.parent === bn && !u.routed);
    const bnUnderPressure = fwdBnCos.some(u =>
      units.some(a => a.side === "atk" && !a.routed && (a.tile === u.tile || ADJ[u.tile].includes(a.tile))));
    if (bnUnderPressure) {
      dr.parent = bn;  // commit: strip RSV prefix
      addLog(`${dr.id} (def reserve) committed to reinforce ${bn}`, "info");
    }
  }
}

// -- Tile ownership & fog -----------------------------------------

function updateOwnership(): void {
  for (let i = 0; i < N_TILES; i++) {
    const atkHere = units.some(u => u.side === "atk" && u.tile === i && !u.routed);
    const defHere = units.some(u => u.side === "def" && u.tile === i && !u.routed);
    if (atkHere && !defHere) tiles[i].owner = "atk";
    else if (defHere && !atkHere) tiles[i].owner = "def";
    // contested or empty: keep last owner (front line stays where it was)
  }
}

function updateFog(): void {
  // Attacker knows: any tile they own, or any tile adjacent to a friendly unit.
  for (let i = 0; i < N_TILES; i++) {
    if (tiles[i].owner === "atk") { knownToAtk[i] = true; continue; }
    if (units.some(u => u.side === "atk" && !u.routed && (u.tile === i || ADJ[u.tile].includes(i)))) {
      knownToAtk[i] = true;
    }
    // tiles once seen stay known (no re-fog)
  }
  // reveal enemy units on known tiles
  for (const d of units.filter(u => u.side === "def" && !u.routed && !u.revealed)) {
    if (knownToAtk[d.tile]) {
      d.revealed = true;
      const ent = d.entrenched ? " [ENTRENCHED]" : "";
      addLog(`Recon: ${d.id} spotted at ${LABELS[d.tile]} (~${Math.round(d.men/50)*50} men, ${d.mg}MG)${ent}`, "recon");
    }
  }
}

// -- Movement: 2 tiles, 1 if entering enemy ZOC -------------------

function inEnemyZOC(tile: number, side: "atk"|"def"): boolean {
  // ZOC projected by revealed enemy units only (hidden defenders don't project ZOC for attacker)
  return ADJ[tile].some(t => units.some(u =>
    u.side !== side && !u.routed && u.tile === t &&
    (side === "def" ? true : u.revealed)
  ));
}

function doMovement(): void {
  for (const a of units.filter(u => u.side === "atk" && !u.routed)) {
    // Support plt follows its parent bn's lead co
    if (a.type === "support") {
      const parentCos = units.filter(u => u.side === "atk" && u.parent === a.parent && u.type === "inf" && !u.routed);
      if (parentCos.length) {
        const lead = parentCos.reduce((best, u) => ROW[u.tile] < ROW[best.tile] ? u : best, parentCos[0]);
        if (a.tile !== lead.tile) {
          const from = a.tile; a.tile = lead.tile;
          addLog(`${a.id} follows ${lead.id} to ${LABELS[a.tile]}`, "move");
        }
      }
      continue;
    }
    // Reserve cos wait until committed
    if (a.parent?.startsWith("RSV/") && !reserveCommitted) {
      addLog(`${a.id} holds in reserve at ${LABELS[a.tile]}`, "info");
      continue;
    }
    // Respect stacking limit: do not enter a tile already at capacity with friendlies.
    // (We treat enemy occupation as the blocker for ZOC; friendly stacking handled in opts sort.)
    // Movement allowance: 2 normally, 1 if starting in enemy ZOC.
    let mp = inEnemyZOC(a.tile, a.side) ? 1 : 2;
    const startedInZOC = mp === 1;
    if (startedInZOC) addLog(`${a.id} in enemy ZOC at ${LABELS[a.tile]} -- move reduced to 1`, "move");

    while (mp > 0) {
      // Tile is a valid destination if: no enemy units there AND friendly count < terrain capacity.
      const friendlyCount = (t: number) =>
        units.filter(u => u.side === a.side && u.tile === t && !u.routed && u.id !== a.id).length;
      const enemyOn = (t: number) =>
        units.some(u => u.side !== a.side && u.tile === t && !u.routed);
      const passable = (t: number) =>
        !enemyOn(t) && friendlyCount(t) < capacityOf(tiles[t].terrain);

      // Prefer forward (toward defender), fall back to lateral, then backward.
      let opts = ADJ[a.tile].filter(t => ROW[t] < ROW[a.tile]).filter(passable);
      if (!opts.length) opts = ADJ[a.tile].filter(t => ROW[t] === ROW[a.tile]).filter(passable);
      if (!opts.length) break;
      // prefer tiles not stacked with friendlies, then better terrain (cover for the assault)
      opts.sort((x, y) => {
        const fx = friendlyCount(x), fy = friendlyCount(y);
        if (fx !== fy) return fx - fy;
        return tiles[y].mul - tiles[x].mul;
      });
      const dest = opts[0];
      const enteringZOC = inEnemyZOC(dest, a.side);
      const from = a.tile;
      a.tile = dest;
      mp--;
      addLog(`${a.id} marches ${LABELS[from]} -> ${LABELS[dest]}${enteringZOC ? " (enters ZOC)" : ""}`, "move");
      // Auto-recon as we march: spot adjacent + own tile
      autoSpot(a);
      // Standard wargame rule: entering enemy ZOC ends movement
      if (enteringZOC) break;
    }
  }
}

function autoSpot(u: Unit): void {
  for (const ti of [u.tile, ...ADJ[u.tile]]) {
    knownToAtk[ti] = knownToAtk[ti] || u.side === "atk";
    for (const e of units.filter(x => x.side !== u.side && x.tile === ti && !x.routed && !x.revealed)) {
      const pen = tiles[ti].terrain === "forest" ? 2 : tiles[ti].terrain === "village" ? 1 : 0;
      const roll = d6() + (ti === u.tile ? 3 : 0);
      if (roll - pen >= 3) {
        e.revealed = true;
        const ent = e.entrenched ? " [ENT]" : "";
        addLog(`${u.id} spots ${e.id} at ${LABELS[ti]}${ent}`, "recon");
      } else if (roll === 1 && ti !== u.tile) {
        const cas = rand(2, 6); applyCas(u, cas);
        addLog(`${u.id} takes fire from concealed position near ${LABELS[ti]}: -${cas} men`, "combat");
      }
    }
  }
}

// -- Assaults -----------------------------------------------------

function doAssaults(): void {
  // Concentrate attacks on as many frontline defenders as possible so that
  // every defending bn is engaged simultaneously -- this neutralises the
  // mutual flanking-fire support each defender otherwise provides.
  const plan = new Map<number, Unit[]>();
  const attackers = units.filter(u => u.side === "atk" && !u.routed && u.type === "inf"
    && !(u.parent?.startsWith("RSV/") && !reserveCommitted));
  const defenderTiles = new Set<number>();
  for (const d of units.filter(u => u.side === "def" && !u.routed && u.revealed)) defenderTiles.add(d.tile);

  // Phase 1: pin -- each revealed defender tile that is adjacent to at least one
  // attacker gets exactly one attacker assigned (pinning it so it can't flank-fire
  // the main effort).
  const unassigned = new Set(attackers.map(a => a.id));
  for (const ti of defenderTiles) {
    const cands = attackers.filter(a => unassigned.has(a.id) && ADJ[a.tile].includes(ti));
    if (!cands.length) continue;
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
      const s = (ti: number) => units.filter(u => u.side === "def" && u.tile === ti && !u.routed).reduce((acc, u) => acc + cv(u), 0);
      return s(y) < s(x) ? y : x;
    });
    for (const a of attackers) {
      if (!unassigned.has(a.id)) continue;
      if (!ADJ[a.tile].includes(weakest)) continue;
      plan.get(weakest)!.push(a);
      unassigned.delete(a.id);
    }
  }

  const capturedTiles = new Set<number>();
  if (plan.size > 0) {
    const posBefore = new Map<string, number>();
    for (const u of units.filter(u => u.side === "atk" && !u.routed)) posBefore.set(u.id, u.tile);
    const attackedTiles = new Set(plan.keys());
    for (const [ti, atks] of plan) {
      const ds = units.filter(u => u.side === "def" && u.tile === ti && !u.routed);
      if (ds.length > 0) resolveCombat(atks, ds, ti, attackedTiles);
    }
    for (const u of units.filter(u => u.side === "atk" && !u.routed)) {
      const prev = posBefore.get(u.id);
      if (prev !== undefined && prev !== u.tile) capturedTiles.add(u.tile);
    }
  }

  // Defender counterattacks -- proactive, not just reactive.
  // A defender will sortie from its tile to strike any adjacent attacker
  // whenever it has at least parity (CV >= 0.9 * enemy CV) and decent morale.
  // Priority targets: (1) attackers on captured friendly tiles, (2) attackers
  // adjacent to multiple defenders (concentrated counter-blow).
  const counterCandidates = units.filter(u => u.side === "def" && !u.routed && u.morale >= 50);
  for (const d of counterCandidates) {
    if (units.some(u => u.side === "atk" && u.tile === d.tile && !u.routed)) continue;
    const adjAtkTiles = ADJ[d.tile].filter(t =>
      units.some(u => u.side === "atk" && u.tile === t && !u.routed));
    if (!adjAtkTiles.length) continue;

    // Score each adjacent attacker-occupied tile and pick the best target.
    adjAtkTiles.sort((x, y) => {
      const score = (t: number) => {
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
    if (cv(d) / Math.max(1, eCV) < 0.9) continue;
    addLog(`${d.id} counterattacks ${enemy.map(e => e.id).join("+")} at ${LABELS[target]}!`, "combat");
    resolveCounterattack(d, enemy, target);
    capturedTiles.delete(target);
  }

  // Defender low-morale retreat (if not in contact)
  for (const d of units.filter(u => u.side === "def" && !u.routed && u.morale < 35)) {
    if (units.some(u => u.side === "atk" && ADJ[d.tile].includes(u.tile) && !u.routed)) continue;
    const bk = ADJ[d.tile].filter(t => ROW[t] < ROW[d.tile]);
    if (!bk.length) continue;
    const from = d.tile; d.tile = bk[rand(bk.length)];
    d.entrenched = false;
    addLog(`${d.id} retreats ${LABELS[from]} -> ${LABELS[d.tile]} (M:${d.morale}, lost entrenchment)`, "move");
  }
}

// -- ARTILLERY (callable from any phase) -------------------------

function fireAtkArt(): void {
  if (atkGuns <= 0) return;
  const tgt = new Set<number>();
  for (const u of units) if (u.side === "def" && u.revealed && !u.routed) tgt.add(u.tile);
  if (tgt.size === 0) return;
  const targets = [...tgt];
  const perTgt = Math.ceil(atkGuns / targets.length);
  let used = 0;
  for (const ti of targets) {
    const guns = Math.min(perTgt, atkGuns - used);
    used += guns;
    for (const d of units.filter(u => u.side === "def" && u.tile === ti && !u.routed)) {
      let cas = 0;
      for (let g = 0; g < guns; g++) cas += Math.floor(rand(3, 8) / tiles[ti].mul);
      applyCas(d, cas);
      const sl = guns >= 7 ? 3 : guns >= 4 ? 2 : guns >= 2 ? 1 : 0;
      d.suppression = Math.max(d.suppression, sl);
      d.morale = clamp(d.morale - rand(3, 7), 0, 100);
      addLog(`394th Art (${guns}x 105mm) -> ${LABELS[ti]}: ${d.id} -${cas} men, ${SUPP[d.suppression]}`, "arty");
    }
  }
}

function fireDefArt(): void {
  if (defGuns <= 0) return;
  const atkTiles: number[] = [];
  for (const u of units) if (u.side === "atk" && u.type === "inf" && !u.routed && !atkTiles.includes(u.tile)) atkTiles.push(u.tile);
  if (atkTiles.length === 0) return;
  const ti = atkTiles[rand(atkTiles.length)];
  const tgts = units.filter(u => u.side === "atk" && u.tile === ti && !u.routed);
  for (const a of tgts) {
    const cas = Math.max(1, Math.floor(rand(2, 5) * defGuns / Math.max(1, tgts.length) / tiles[ti].mul));
    applyCas(a, cas);
    addLog(`1028th Art (${defGuns}x 76mm) -> ${LABELS[ti]}: ${a.id} -${cas} men`, "arty");
  }
}

function counterBattery(): void {
  if (atkGuns > 0 && defGuns > 0 && d6() >= 5) {
    defGuns--; addLog(`Counter-battery: 1028th Art loses 1 gun (${defGuns} left)`, "arty");
  }
  if (defGuns > 0 && atkGuns > 0 && d6() >= 5) {
    atkGuns--; addLog(`Counter-battery: 394th Art loses 1 gun (${atkGuns} left)`, "arty");
  }
}


function resolveCombat(atks: Unit[], defs: Unit[], ti: number, attackedTiles: Set<number>): void {
  const tile = tiles[ti];
  let atkCV = atks.reduce((s, a) => s + cv(a), 0);
  const defCV = defs.reduce((s, d) => s + cv(d), 0);
  if (atkCV <= 0 || defCV <= 0) return;

  const isFlankAssault = atks.some(a => ROW[a.tile] === ROW[ti] && a.tile !== ti);
  const flankMul = isFlankAssault ? 1.2 : 1.0;
  if (isFlankAssault) addLog(`  flanking assault: x1.2 attacker CV`, "combat");

  let flankCV = 0;
  const flankFrom: string[] = [];
  for (const adj of ADJ[ti]) {
    if (attackedTiles.has(adj)) continue;
    for (const fd of units.filter(u => u.side === "def" && u.tile === adj && !u.routed)) {
      const contrib = Math.floor(cv(fd) * 0.3);
      if (contrib > 0) { flankCV += contrib; flankFrom.push(LABELS[adj]); }
    }
  }
  const totalDefCV = defCV + flankCV;
  if (flankCV > 0) addLog(`  flanking fire from ${[...new Set(flankFrom)].join(",")}: +CV:${flankCV}`, "combat");

  const entMul = defs.some(d => d.entrenched) ? 1.3 : 1.0;
  if (entMul > 1) addLog(`  defenders entrenched: x${entMul} bonus`, "combat");

  const tAmen = atks.reduce((s, a) => s + a.men, 0);
  const rawRatio = (atkCV * flankMul) / totalDefCV;
  const avgS = defs.reduce((s, d) => s + d.suppression, 0) / defs.length;
  const eff = rawRatio / (tile.mul * entMul) + avgS * 0.15;

  let ar: number, dr: number;
  if      (eff >= 3.0) { ar = 0.015; dr = 0.10;  }
  else if (eff >= 2.0) { ar = 0.03;  dr = 0.07;  }
  else if (eff >= 1.5) { ar = 0.05;  dr = 0.05;  }
  else if (eff >= 1.0) { ar = 0.07;  dr = 0.03;  }
  else                 { ar = 0.09;  dr = 0.015; }

  const aLoss = Math.max(1, Math.floor(tAmen * ar * (0.7 + Math.random() * 0.6)));
  for (const a of atks) {
    const l = Math.max(1, Math.round(aLoss * (a.men / Math.max(1, tAmen))));
    applyCas(a, l);
    a.morale = clamp(a.morale - Math.ceil(l / 10), 0, 100);
    if (a.morale <= 15 || a.men < 80) { a.routed = true; addLog(`  X ${a.id} ROUTS (${a.men} men)`, "result"); }
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
        const from = d.tile; d.tile = rear[rand(rear.length)];
        d.entrenched = false;
        addLog(`  ${d.id} RETREATS ${LABELS[from]} -> ${LABELS[d.tile]} (M:${d.morale})`, "move");
      } else {
        d.routed = true; addLog(`  X ${d.id} ROUTS (${d.men} flee)`, "result");
      }
    }
  }

  const alive = atks.filter(a => !a.routed);
  // Defenders that remain ON the contested tile (retreated/routed ones have already left).
  const dHold = defs.filter(d => !d.routed && d.tile === ti);
  let out: string;
  if (!dHold.length && alive.length) {
    // Tile is clear -- attackers move in and occupy.
    for (const a of alive) {
      a.tile = ti;
      for (const r of units.filter(u => u.type === "recon" && u.parent === a.id && !u.routed)) r.tile = ti;
    }
    out = "TAKES " + LABELS[ti];
  } else if (alive.length) {
    // Defenders still hold -- attackers stay on their own tile, exchange fire.
    if      (eff >= 2.0) out = "hammers " + LABELS[ti];
    else if (eff >= 1.4) out = "presses " + LABELS[ti];
    else                 out = "repelled from " + LABELS[ti];
  } else {
    out = "repelled from " + LABELS[ti];
  }

  addLog(`${atks.map(a => a.id).join("+")} -> ${LABELS[ti]}: ${out}  [CV ${rawRatio.toFixed(1)}:1 ter x${(tile.mul * entMul).toFixed(1)} -> ${eff.toFixed(1)} eff]  A-${aLoss} D-${dLoss}`, "combat");
}

function resolveCounterattack(ctr: Unit, enemy: Unit[], ti: number): void {
  const tile = tiles[ti];
  const cCV = cv(ctr);
  const eCV = enemy.reduce((s, u) => s + cv(u), 0);
  if (cCV <= 0 || eCV <= 0) return;

  // no entrenchment — attackers just arrived
  const ratio = cCV / eCV;
  const eff = ratio / tile.mul;

  let cr: number, er: number;
  if      (eff >= 2.0) { cr = 0.02; er = 0.07; }
  else if (eff >= 1.2) { cr = 0.04; er = 0.04; }
  else if (eff >= 0.8) { cr = 0.06; er = 0.025; }
  else                 { cr = 0.08; er = 0.015; }

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
    if (e.morale <= 15 || e.men < 80) { e.routed = true; addLog(`  X ${e.id} ROUTS from counterattack`, "result"); }
  }

  const eAlive = enemy.filter(e => !e.routed);
  let result: string;
  if (!eAlive.length) {
    ctr.tile = ti; ctr.entrenched = false;
    result = `RETAKES ${LABELS[ti]}`;
  } else if (eff >= 1.3 && d6() >= 4) {
    // Try to push every surviving attacker off the tile. Only move the counter-attacker
    // onto the tile if every attacker actually had somewhere to retreat to -- otherwise
    // we'd end up with friendly + enemy units co-located on the same hex.
    const pushed: Unit[] = [];
    for (const e of eAlive) {
      const back = ADJ[e.tile].filter(t => ROW[t] > ROW[e.tile] &&
        !units.some(u2 => u2.side === "def" && u2.tile === t && !u2.routed));
      if (back.length) {
        e.tile = back[rand(back.length)];
        pushed.push(e);
      }
    }
    if (pushed.length === eAlive.length) {
      ctr.tile = ti; ctr.entrenched = false;
      result = `drives enemy from ${LABELS[ti]}`;
    } else {
      result = `pushes ${LABELS[ti]} but enemy holds`;
    }
  } else {
    result = `counterattack repelled`;
  }

  if (ctr.morale <= 25 || ctr.men < 100) {
    ctr.routed = true; addLog(`  X ${ctr.id} ROUTS after counterattack`, "result");
  }

  addLog(`  ${ctr.id} -> ${LABELS[ti]}: ${result}  [CV ${ratio.toFixed(1)}:1 -> ${eff.toFixed(1)} eff]  C-${cLoss} E-${eLoss}`, "combat");
}

// -- Victory ------------------------------------------------------

function checkEnd(): void {
  // Win = reaching row A (index 0), the deep defender rear. Row B (1) is the main line -- fighting there is expected.
  const breach = units.filter(u => u.side === "atk" && !u.routed && u.type === "inf" && ROW[u.tile] === 0);
  const defAlive = units.filter(u => u.side === "def" && !u.routed);
  const atkAlive = units.filter(u => u.side === "atk" && !u.routed && u.type === "inf");
  if (breach.length)      { over = true; addLog(`\n* ATTACKER VICTORY -- ${breach[0].id} broke through to ${LABELS[breach[0].tile]}`, "result"); }
  else if (!defAlive.length) { over = true; addLog("\n* ATTACKER VICTORY -- all defenders routed", "result"); }
  else if (!atkAlive.length) { over = true; addLog("\n* DEFENDER VICTORY -- all attackers routed", "result"); }
  else if (turn >= 24)       { over = true; addLog(`\n* STALEMATE -- nightfall (${tStr(turn)})`, "result"); }
}

// =================================================================
//  RENDERING
// =================================================================

function renderFrame(): void {
  const f = steps[stepIdx];
  renderGrid(f);
  renderOOB(f.units);
  renderLog(f.logEnd);
  renderPhaseBar(f);
  renderControls();
  const res = $("results");
  if (stepIdx === steps.length - 1 && f.over) { renderResults(); res.classList.remove("hidden"); }
  else res.classList.add("hidden");
}

// -- Hex grid + off-map -------------------------------------------

function renderGrid(f: Frame): void {
  const g = $("hex-grid");
  let html = "";

  // off-map defender art (top)
  html += `<div class="offmap offmap-def">`
    + `<div class="offmap-sym">${symSvg(SIDC_DEF_ART, 24)}</div>`
    + `<div class="offmap-info"><div class="offmap-nm">1028th Art Btry</div>`
    + `<div class="offmap-det">${f.defGuns}x 76mm | ${f.defGuns > 0 ? "READY" : "DESTROYED"}</div></div></div>`;

  // row-letter zone labels positioned on the LEFT of the grid, one per row
  for (let r = 0; r < ROWS; r++) {
    const y = MAP_PAD_TOP + r * RS + Math.floor(HH / 2) - 6;
    // defender side = rows 0-2, attacker side = rows 6-9, mid = rows 3-5
    const cls = r <= 2 ? "zone zone-def" : r >= 6 ? "zone zone-atk" : "zone zone-nml";
    html += `<div class="${cls} zone-row" style="left:6px;top:${y}px;width:${MAP_PAD_LEFT - 14}px">${ROW_NAMES[r]} — ${ZONES[r]}</div>`;
  }

  // hexes
  for (let i = 0; i < N_TILES; i++) {
    const p = HP[i], t = tiles[i];
    const onTile = f.units.filter(u => u.tile === i && !u.routed);
    const hasAtk = onTile.some(u => u.side === "atk");
    const hasDef = onTile.some(u => u.side === "def" && u.revealed);
    const contested = hasAtk && hasDef;
    const owner = f.owners[i];
    const known = f.known[i];
    const ownerCls = owner === "atk" ? " own-atk" : owner === "def" ? " own-def" : " own-neu";
    const fogCls = !known ? " hex-fog" : "";

    html += `<div class="hex-wrap" data-tile="${i}" style="left:${p.x}px;top:${p.y}px;width:${HW}px;height:${HH}px;cursor:pointer">`
      + `<div class="hex-bdr${contested ? " contested" : ""}${ownerCls}" style="clip-path:${CLIP}"></div>`
      + `<div class="hex-fill ter-${t.terrain}${fogCls}" style="clip-path:${CLIP}"></div>`
      + `<div class="hex-info" style="clip-path:${CLIP}">`
      +   `<span class="hex-id">${LABELS[i]}</span>`
      +   `<span class="hex-ter">${known ? esc(t.terrain) + " x" + t.mul : "???"}</span>`
      + `</div>`
      + `<div class="hex-units">${mkUnits(onTile, f.reconOut, known)}</div>`
      + `</div>`;
  }

  // recon arrows (parent -> recon tile) during recon phase
  if (f.reconOut) {
    const rcns = f.units.filter(u => u.type === "recon" && !u.routed && u.parent);
    for (const r of rcns) {
      const par = f.units.find(u => u.id === r.parent);
      if (!par || par.tile === r.tile) continue;
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
  g.querySelectorAll<HTMLElement>(".hex-wrap").forEach((hw: HTMLElement) => {
    hw.addEventListener("click", () => renderTileInfo(Number(hw.dataset.tile), steps[stepIdx]));
  });
}

function renderTileInfo(ti: number, f: Frame): void {
  const panel = $("tile-panel");
  const t = tiles[ti];
  const known = f.known[ti];
  const owner = f.owners[ti];
  const ownerLabel = owner === "atk" ? "Attacker" : owner === "def" ? "Defender" : "Neutral";
  const onTile = f.units.filter(u => u.tile === ti);
  const visibleUnits = onTile.filter(u => !u.routed && (u.side === "atk" || known));
  const routedUnits  = onTile.filter(u =>  u.routed && (u.side === "atk" || known));

  $("tp-title").textContent = `Hex ${LABELS[ti]}`;

  let body = `<span class="tp-owner ${owner === "atk" ? "own-atk" : owner === "def" ? "own-def" : "own-neu"}">${ownerLabel}</span>`;
  body += `<div class="tp-terrain">`
    + (known
      ? `<b>${t.terrain.charAt(0).toUpperCase() + t.terrain.slice(1)}</b> &nbsp;·&nbsp; Defence ×${t.mul}`
      : `<span class="tp-fog">Terrain unknown (fog of war)</span>`)
    + `</div>`;

  const renderUnitBlock = (u: Unit, dim = false) => {
    const nameCls = u.routed ? " routed" : "";
    const sup = ["—", "Disrupted", "Suppressed", "Pinned"][u.suppression] ?? "—";
    const status = u.routed ? "ROUTED"
      : u.entrenched ? "Entrenched"
      : u.suppression > 0 ? sup
      : "Ready";
    const sidcSvg = symSvg(u.sidc, 28);
    return `<div class="tp-unit" style="${dim ? "opacity:.45" : ""}">`
      + `<div class="tp-unit-sym">${sidcSvg}</div>`
      + `<div class="tp-unit-body">`
      +   `<div class="tp-unit-name${nameCls}">${esc(u.id)}</div>`
      +   `<div class="tp-unit-stats">Men: <b>${u.men}</b> &nbsp;MG: <b>${u.mg}</b> &nbsp;Mor: <b>${u.mortar}</b> &nbsp;AT: <b>${u.at}</b> &nbsp;CV: <b>${cv(u).toFixed(1)}</b></div>`
      +   `<div class="tp-unit-stats">Morale: <b>${u.morale}</b></div>`
      +   `<div class="tp-unit-status">${status}</div>`
      + `</div></div>`;
  };

  if (!visibleUnits.length && !routedUnits.length) {
    body += `<div class="tp-fog">No units</div>`;
  } else {
    if (visibleUnits.length) {
      body += `<div class="tp-section">Units present</div>`;
      body += visibleUnits.map(u => renderUnitBlock(u)).join("");
    }
    if (routedUnits.length) {
      body += `<div class="tp-section">Routed (withdrawing)</div>`;
      body += routedUnits.map(u => renderUnitBlock(u, true)).join("");
    }
  }

  const adjUnits: Unit[] = [];
  for (const adj of ADJ[ti]) {
    for (const u of f.units.filter(u => u.tile === adj && !u.routed && (u.side === "atk" || f.known[adj]))) {
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

function mkUnits(on: Unit[], reconOut: boolean, tileKnown: boolean): string {
  // Hide all enemy units on unknown tiles entirely. Friendly units always shown.
  const visible = on.filter(u => !u.routed && (u.side === "atk" || tileKnown));
  const sorted = [...visible].sort((a, b) => {
    if (a.side !== b.side) return a.side === "def" ? -1 : 1;
    return 0;
  });
  // counter-game stacking: each successive unit cascades down-right on top of the previous one
  return sorted.map((u, i) => mkUnit(u, 10 + i, i)).join("");
}

function mkUnit(u: Unit, z: number, stack: number = 0): string {
  const fog = !u.revealed && u.side === "def";
  const sup = u.suppression > 0 ? ` u-s${u.suppression}` : "";
  const cls = u.side === "atk" ? "u-atk" : "u-def";
  const szPx = u.size === "bn" ? 18 : u.size === "co" ? 16 : 14;

  if (fog) {
    return `<div class="unit ${cls} u-fog" style="z-index:${z};--sx:${stack}" title="Unidentified enemy unit">`
      + `<div class="u-sym">${symSvg(SIDC_UNK, 16)}</div>`
      + `<div class="u-label">???</div></div>`;
  }

  const ent = u.entrenched ? " ENT" : "";
  const st = u.suppression > 0 ? " " + SUPP[u.suppression].substring(0, 4) : "";
  const tip = `${u.id}  ${u.men} men  CV:${cv(u)}  ${u.mg}MG ${u.mortar}Mor ${u.at}AT  M:${u.morale}  ${SUPP[u.suppression]}${u.entrenched ? "  ENTRENCHED" : ""}`;
  const rcn = u.type === "recon";

  return `<div class="unit ${cls}${sup}${rcn ? " u-rcn" : ""}" style="z-index:${z};--sx:${stack}" title="${esc(tip)}">`
    + `<div class="u-sym">${symSvg(u.sidc, szPx)}</div>`
    + `<div class="u-label"><div class="u-name">${u.id}</div>${u.men} CV:${cv(u)}${ent}${st}</div>`
    + `</div>`;
}

// -- OOB tables ---------------------------------------------------

function renderOOB(fu: Unit[]): void {
  // Attacker bns
  let aHtml = "";
  for (const bn of fu.filter(u => u.side === "atk")) aHtml += oobRow(bn, true);
  $("oob-a").innerHTML = aHtml;

  // Defender
  $("oob-d").innerHTML = fu.filter(u => u.side === "def").map(u => oobRow(u, u.revealed || u.routed)).join("");
}

function oobRow(u: Unit, known: boolean): string {
  if (!known)
    return `<tr class="oob-tr fog"><td class="oob-bn">${esc(u.id)}</td><td colspan="4" class="oob-unk">???</td></tr>`;
  const cls = u.routed ? "oob-tr rt" : "oob-tr";
  const sup = u.suppression > 0 && !u.routed ? ` s${u.suppression}` : "";
  return `<tr class="${cls}${sup}">`
    + `<td class="oob-bn">${esc(u.id)}${u.entrenched ? " E" : ""}</td>`
    + `<td class="oob-n">${u.routed ? "--" : u.men}</td>`
    + `<td class="oob-n oob-cv">${cv(u)}</td>`
    + `<td class="oob-n">${u.morale}</td>`
    + `<td class="oob-h">${LABELS[u.tile]}</td>`
    + `</tr>`;
}

// -- Log ----------------------------------------------------------

function renderLog(logEnd: number): void {
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

function renderPhaseBar(f: Frame): void {
  const label = f.over ? "RESULT" : tStr(f.turn);
  $("phase").textContent = label;
  $("step").textContent = `${stepIdx + 1} / ${steps.length}`;
  ($("progress") as HTMLElement).style.width = `${(stepIdx / Math.max(1, steps.length - 1)) * 100}%`;
}

function renderControls(): void {
  ($("prev") as HTMLButtonElement).disabled = stepIdx <= 0;
  ($("next") as HTMLButtonElement).disabled = stepIdx >= steps.length - 1;
  ($("skip") as HTMLButtonElement).disabled = stepIdx >= steps.length - 1;
}

// -- Results ------------------------------------------------------

function renderResults(): void {
  const ini = steps[0].units;
  const fin = steps[steps.length - 1].units;

  let verdict = "";
  for (let i = fullLog.length - 1; i >= 0; i--) {
    if (fullLog[i].type === "result" && fullLog[i].text.includes("*")) { verdict = fullLog[i].text; break; }
  }

  const fs = steps[steps.length-1];
  let h = `<div class="res-v">${esc(verdict)}</div>`;
  const minutes = (fs.turn - 1) * 15;
  const dur = `${Math.floor(minutes / 60)}h ${minutes % 60}m`;
  h += `<div class="res-sub">${tStr(1)}–${tStr(fs.turn)} (${dur})  |  Atk Art: ${fs.atkGuns}/${steps[0].atkGuns} guns  |  Def Art: ${fs.defGuns}/${steps[0].defGuns} guns</div>`;

  for (const side of ["atk", "def"] as const) {
    const label = side === "atk" ? "394th Infantry Regiment" : "1028th Rifle Regiment";
    h += `<div class="res-rgt">${esc(label)}</div>`;
    h += `<table class="res-t"><thead><tr><th>Unit</th><th>Type</th><th>Men</th><th></th><th>End</th><th>Lost</th><th>MG</th><th>Mor</th><th>AT</th><th>CV</th></tr></thead><tbody>`;

    const sideUnits = ini.filter(u => u.side === side);
    for (const iu of sideUnits) {
      const fu = fin.find(u => u.id === iu.id);
      if (!fu) continue;
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
    h += `<tr class="res-tot"><td>Total</td><td></td><td>${s0}</td><td>-></td><td>${s1}</td><td class="res-lost">-${s0-s1}</td><td></td><td></td><td></td><td>${cv0}->${cv1}</td></tr>`;
    h += `</tbody></table>`;
  }
  $("results").innerHTML = h;
}

// -- Wire up ------------------------------------------------------

$("prev").addEventListener("click", () => { if (stepIdx > 0) { stepIdx--; renderFrame(); } });
$("next").addEventListener("click", () => { if (stepIdx < steps.length - 1) { stepIdx++; renderFrame(); } });
$("skip").addEventListener("click", () => { stepIdx = steps.length - 1; renderFrame(); });
$("tp-close").addEventListener("click", () => $("tile-panel").classList.remove("open"));
$("reset").addEventListener("click", initBattle);

// Keyboard hotkeys
document.addEventListener("keydown", (e: KeyboardEvent) => {
  if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) return;
  switch (e.key) {
    case "ArrowRight": case "d": case ".":
      if (stepIdx < steps.length - 1) { stepIdx++; renderFrame(); }
      e.preventDefault(); break;
    case "ArrowLeft": case "a": case ",":
      if (stepIdx > 0) { stepIdx--; renderFrame(); }
      e.preventDefault(); break;
    case "End": case "s":
      stepIdx = steps.length - 1; renderFrame();
      e.preventDefault(); break;
    case "Home": case "w":
      stepIdx = 0; renderFrame();
      e.preventDefault(); break;
    case "r":
      initBattle();
      e.preventDefault(); break;
  }
});

initBattle();
