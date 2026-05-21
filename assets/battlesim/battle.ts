// ── Types ────────────────────────────────────────────────
interface Division {
  name: string;
  manpower: number;
  rifles: number;
  machine_guns: number;
  artillery: number;
  anti_tank_guns: number;
  tanks: number;
  trucks: number;
}

interface RoundEntry {
  phase: string;
  attacker_action: string;
  defender_action: string;
  attacker_losses: number;
  defender_losses: number;
}

interface SideData {
  division: string;
  before: Division;
  after: Division;
  cv_before: number;
  cv_after: number;
}

interface BattleData {
  attacker: SideData;
  defender: SideData;
  rounds: RoundEntry[];
}

// ── Helpers ──────────────────────────────────────────────
function rand(a: number, b?: number): number {
  if (b === undefined) return Math.floor(Math.random() * a);
  return a + Math.floor(Math.random() * (b - a + 1));
}

function esc(s: string): string {
  const el = document.createElement("span");
  el.textContent = s;
  return el.innerHTML;
}

function clone<T>(obj: T): T { return JSON.parse(JSON.stringify(obj)); }

function el(id: string): HTMLElement { return document.getElementById(id)!; }

// ── Divisions ───────────────────────────────────────────
const DIVISIONS: Record<string, Division> = {
  soviet_rifle_1944: {
    name: "Soviet Rifle Division", manpower: 9500, rifles: 7000,
    machine_guns: 180, artillery: 48, anti_tank_guns: 18, tanks: 0, trucks: 120,
  },
  german_infantry_1944: {
    name: "German Infantry Division", manpower: 12500, rifles: 9000,
    machine_guns: 250, artillery: 72, anti_tank_guns: 24, tanks: 0, trucks: 900,
  },
  panzer_division: {
    name: "11th Panzer Division", manpower: 14200, rifles: 6800,
    machine_guns: 320, artillery: 64, anti_tank_guns: 48, tanks: 180, trucks: 1240,
  },
};

// organization knowledge
// squad - large enough to maneuver alone. small enough for one guy to lead it
// platoon - achieves fire and maneuver. small enough to keep track of all people
// company - largest frontline unit (500 m depth. so each platoon can reach with small arms. 150 m distance so each platoon doesnt get hit by artillery)
// battalion - smallest independent. contains food
// regiment - often contains its own type. Inf reg, contains inf bat
// division - contains mix of regiments. Combined arms
// Operational:
// corps; contains multiple division. and unique support units. like heavy arty. that can be assigned to any sub unit
// army - 


const UNIT_KEYS: [keyof Division, string][] = [
  ["manpower",       "Manpower"],
  ["rifles",         "Rifles"],
  ["tanks",          "Tanks"],
  ["machine_guns",   "Machine Guns"],
  ["artillery",      "Artillery"],
  ["anti_tank_guns", "Anti-Tank Guns"],
  ["trucks",         "Trucks"],
];

function computeCV(d: Division): number {
  return Math.round(
    d.manpower / 500 + d.rifles / 200 + d.tanks * 2 +
    d.artillery * 0.8 + d.anti_tank_guns * 0.6 + d.machine_guns * 0.3
  );
}

// ═════════════════════════════════════════════════════════
//  BATTLE LOGIC — edit this, save, rebuild, refresh
// ═════════════════════════════════════════════════════════

// ── Scenario constants ───────────────────────────────────
// Tile size    : 10 km × 10 km  (one regiment-width sector)
// Tactical level: regiment  — each Division contains ~3 regiments;
//                each battle here represents one regiment attacking
//                one defending regiment on a single tile.
// Manpower needed to hold a tile: see garrisonThresholds below.
const TILE_KM        = 10;         // km per hex edge
const TACTICAL_LEVEL = "regiment"; // regiment-vs-regiment engagement
const RETREAT_THRESHOLD = 0.25;    // defender retreats if ≥25 % of pre-assault rifles lost
const REPEL_THRESHOLD   = 0.20;    // attacker breaks off if ≥20 % of pre-assault rifles lost

// ── Time helper ─────────────────────────────────────────
// Convert elapsed minutes (since 0500) to a HHMM string
function fmtTime(elapsed: number): string {
  const total = 5 * 60 + elapsed;
  return String(Math.floor(total / 60)).padStart(2, '0') + String(total % 60).padStart(2, '0');
}

// Named ground objectives the battalion CO references in orders
const OBJECTIVES = ["Village Nord", "Hill 312", "Treeline Alpha", "Crossroads W4"];

function simulate(attacker: Division, defender: Division): { attacker: Division; defender: Division; rounds: RoundEntry[] } {
  const rounds: RoundEntry[] = [];
  const TURNS_RECON   = 4;   // 0500–0600  (1 h, 4 × 15 min)
  const TURNS_ARTY    = 4;   // 0600–0700  (1 h, 4 × 15 min)
  const TURNS_ASSAULT = 8;   // 0700–0900  (2 h, 8 × 15 min)
  let elapsed = 0;           // minutes elapsed since 0500

  // ── Phase 1: Recon  0500–0600 ─────────────────────────
  // Platoon-scale probes forward to identify enemy positions and strongpoints
  const reconPlt = Math.max(1, Math.floor(attacker.rifles / 10 / 4 / TURNS_RECON));
  const reconOrders = [
    (f: string) => `I/394 Cmd to Recon Plt: Advance to ${f} — identify MG positions and trench lines`,
    (f: string) => `I/394 Cmd to Recon Plt: Recon in force toward ${f}; mark wire, minefields, sector boundary`,
    (f: string) => `I/394 Cmd to Recon Plt: Probe ${f} perimeter — report battalion strength and left-flank gap`,
    (f: string) => `I/394 Cmd to all platoons: Withdraw recon elements; compile intel — assault orders to follow`,
  ];
  const reconDefActions = [
    "Outpost fires warning shots at 600 m; pulls back to main line",
    "MG opens up at 400 m — patrol pins forward squad",
    "Mortar barrage on approach route; scouts fall back to report",
    "Reports to CO: enemy probing in strength — all positions manned",
  ];
  for (let t = 0; t < TURNS_RECON; t++) {
    const obj = OBJECTIVES[t % OBJECTIVES.length];
    let atkL = 0, defL = 0;
    for (let i = 0; i < reconPlt; i++) {
      const roll = rand(6);
      if (roll === 0) { const k = rand(2, 5); attacker.rifles -= k; atkL += k; }
      else if (roll === 1) { const a = rand(1, 3), d = rand(1, 2); attacker.rifles -= a; defender.rifles -= d; atkL += a; defL += d; }
    }
    rounds.push({ phase: fmtTime(elapsed),
      attacker_action: reconOrders[t](obj),
      defender_action: reconDefActions[t],
      attacker_losses: atkL, defender_losses: defL });
    elapsed += 15;
  }

  // ── Phase 2: Artillery prep  0600–0700 ─────────────────
  // Batteries fire in rotation: registration → fire for effect → shift → lift
  const batteries = Math.floor(attacker.artillery / 6);
  const salvosPerTurn = Math.max(1, Math.ceil(batteries / 2));
  const artyOrders = [
    (f: string) => `394th Art Bn Cmd: Battery 1+2 — registration fire on ${f}; observe and correct`,
    (f: string) => `394th Art Bn Cmd: Fire for effect — suppress ${f} strongpoint; battery 3 on left treeline`,
    (f: string) => `394th Art Bn Cmd: Shift fires to ${f}; battery 1 neutralise MG nest at grid 443`,
    (f: string) => `394th Art Bn Cmd: Final protective fires — all batteries on assault axis; lift on green signal`,
  ];
  const artyDefActions = [
    "Platoons seek cover in trenches; MGs displaced to alternate positions",
    "1028th loses comms — runners out; squads hunker in foxholes",
    "Forward platoon abandons exposed position; pulls back 200 m",
    "Platoons reform in defilade; await assault — positions holding",
  ];
  for (let t = 0; t < TURNS_ARTY; t++) {
    const obj = OBJECTIVES[(TURNS_RECON + t) % OBJECTIVES.length];
    let artL = 0;
    for (let i = 0; i < salvosPerTurn; i++) { const hit = rand(5); defender.rifles -= hit; artL += hit; }
    rounds.push({ phase: fmtTime(elapsed),
      attacker_action: artyOrders[t](obj),
      defender_action: artyDefActions[t],
      attacker_losses: 0, defender_losses: artL });
    elapsed += 15;
  }

  // ── Phase 3: Assault  0700–0900 ────────────────────────
  // Battalions advance in 15-min slices; retreat/repel checked each turn
  const preAssaultDefRifles = defender.rifles;
  const preAssaultAtkRifles = attacker.rifles;
  let assaultAtk = 0, assaultDef = 0;
  const assaultOrders = [
    (f: string) => `I/394 Cmd: 1st Coy — fix defenders at ${f}; 2nd Coy provide base of fire on right flank`,
    (f: string) => `I/394 Cmd: 2nd Coy — flank through treeline; seize ${f} from the north`,
    (f: string) => `I/394 Cmd: Commit 3rd Coy through gap at ${f} — do not allow defenders to reform`,
    (f: string) => `I/394 Cmd: Weapons Plt — suppress MG nest at ${f}; assault team follows on green smoke`,
    (f: string) => `I/394 Cmd: All companies push to ${f}; 394th Art shifts to phase line BAKER`,
    (f: string) => `I/394 Cmd: 1st Coy break into ${f} — grenades first, then clear house to house`,
    (f: string) => `I/394 Cmd: Consolidate ${f} — 3rd Coy dig in; prepare for immediate counterattack`,
    (f: string) => `I/394 Cmd: 2nd Coy advance to next phase line; 1st Coy mop up ${f}`,
  ];
  const assaultDefActions = [
    "Platoons hold fire until 200 m — disciplined volley, then MGs open up",
    "MGs concentrate on gap — flanking fire catches assault element in the open",
    "Forward platoon falls back to reserve trench; MGs cover withdrawal",
    "Counterattack group forms behind ridge — CO: hold at all costs",
    "All squads — no withdrawal without order; hold and fire",
    "Reserve platoon plugs left flank — gap closed under fire",
    "Defenders call 1028th batteries — fires on own wire to break assault",
    "Line bending but holding — CO: every man fights in place",
  ];
  for (let t = 0; t < TURNS_ASSAULT; t++) {
    const obj = OBJECTIVES[(TURNS_RECON + TURNS_ARTY + t) % OBJECTIVES.length];
    const defSq = Math.max(1, Math.floor(defender.rifles / 10 / TURNS_ASSAULT));
    const atkSq = Math.max(1, Math.floor(attacker.rifles / 10 / TURNS_ASSAULT));
    let tAtk = 0, tDef = 0;
    for (let i = 0; i < defSq; i++) { const h = rand(6); attacker.rifles -= h; tAtk += h; }
    for (let i = 0; i < atkSq; i++) { const h = rand(4); defender.rifles -= h; tDef += h; }
    assaultAtk += tAtk; assaultDef += tDef;

    const defCasRate = preAssaultDefRifles > 0 ? assaultDef / preAssaultDefRifles : 0;
    const atkCasRate = preAssaultAtkRifles > 0 ? assaultAtk / preAssaultAtkRifles : 0;

    if (defCasRate >= RETREAT_THRESHOLD) {
      const penalty = Math.floor(defender.manpower * 0.05);
      defender.manpower = Math.max(0, defender.manpower - penalty);
      defender.rifles   = Math.max(0, defender.rifles - Math.floor(defender.rifles * 0.05));
      rounds.push({ phase: fmtTime(elapsed),
        attacker_action: `I/394 Cmd: '${obj} is ours — all companies advance and consolidate the position!'`,
        defender_action: `1028th line collapses — retreats under fire (−${penalty} men rout penalty)`,
        attacker_losses: tAtk, defender_losses: tDef + penalty });
      elapsed += 15;
      break;
    }
    if (atkCasRate >= REPEL_THRESHOLD) {
      const penalty = Math.floor(attacker.manpower * 0.03);
      attacker.manpower = Math.max(0, attacker.manpower - penalty);
      attacker.rifles   = Math.max(0, attacker.rifles - Math.floor(attacker.rifles * 0.03));
      rounds.push({ phase: fmtTime(elapsed),
        attacker_action: `I/394 Cmd: 'Break off — assault too costly; consolidate on start line' (−${penalty} men)`,
        defender_action: "1028th holds — fires into withdrawing attackers",
        attacker_losses: tAtk + penalty, defender_losses: tDef });
      elapsed += 15;
      break;
    }
    rounds.push({ phase: fmtTime(elapsed),
      attacker_action: assaultOrders[t % assaultOrders.length](obj),
      defender_action: assaultDefActions[t % assaultDefActions.length],
      attacker_losses: tAtk, defender_losses: tDef });
    elapsed += 15;
  }


  // we assume spread everywhere.

  // squad attacks house
  // platoon attacks hill, trench segment
  // company takes a trench
  // battalion takes a village
  // regiment takes a town
  // division takes a city district
  // corps takes a city
  
  
  // close combat assault 

  // terrain should have something
  // we need fire superiority. to suppress
  // wihtout fire support. that is a 2 mgs so 2 squads to engage 1. 2
  // 3 : 1 on attack. or 6 : 1 or 10 : 1 if maginot.
  // artillery / mortar reduces this to 3 : 1


  // logic is as follows. An attacker spearheads into positioreducesns. 
  // A defender knows this and makes a frontline thin. Then reinforces where spearhead should be with reinforcements to patch other weak areas
  

  // now for defense thresholds. without buildup
  //  1K: half battalion. observation only. march through it 
  //  2K: 1 battalion strength regiment, can delay 12-24hrs (kampfgruppe 45)
  //  5K: 1 regiment. hold probing attacks and unsupported assults. But prepared attack it falls down (soviet rifle quiet sector)
  // 10K: 1 division. 2 : 1 repel. 3 : 1 hold for a day (normandy)
  // 15K: 1 division + attachments. 4 : 1 (cassino / stalingrad tractor factory)

  // from rifle company handbook fm 7-10 1944 
  // move
  // day / night. 

  // attack
  // battalion   has reconned the are
  // approach march (day). 300 yds max split / less if woods. (visibile). Wpn squad at back. Mortar follow each platoon. patrols and scouts at side and front
  // march. if both platoon takes their. then reserve moves
  // one platoon at a time. unless faster is important
  // zone of attack 


  // defense    
  // frontage: poor observation / field of fire:  500 yds (woods)
  // frontage: good observation / field of fire: 1000 yds (plains/fields)
  // unlikely to be attacked: above 1000 yds
  // depth: max 500 yds. each unit can reach eachother in small arms (m1 range is 500) (rifle support range)
  // depth: min 150 yds. each unit should not be hit by same artillery

  // defense. hold: fire at effective range. delay: Fire above effective range. 
  

  // day: hold woods (few at field)
  // night: hold fields (few in woods
  
  // platoon defense
  // (FRONTLINE) repel with range
  // (SUPPORT) support frontline, limit penetration, counterattack, protect flanks
  // frontage (by fire ): 250 yds (woods). 500 yds (fields). 500+ (fields, clear vision)
  // frontage (physical): 300 yds (rest is by fire)
  
  // squad deploymnet: flanks so they have mutual overlap in middle
  // 60 mm deployment: arm and hand reach. 100 yds
  // defense: trench or foxhole
  //    

  // so i guess each unit is one o fthe following
  // attack / defense / support
  // move / hold / attack


  // regiment defense   
  // frontage: 2000 yds (woods). 5000 yds (field). 
  // battalion defense:
  // frontage: 1000 yds (woods). 2000 yds (field). 2500 yds (open field) 3500 yds (swamp/stream)
  // depth: 800-1400 yds

  // clearing woods / deep  defense (good observability by enemy), camuflage  


  // supply classes

  // field fortifications
  // 02K: 04K power. 
  // 05K: 10K power. tobruk
  // 10K: overkill unless kursk


  // bunkers
  // 02K: 05K power held for weeks (iwo jima)
  // 05K: 15K seelow. 3 day delay 1 mil soviets 
  // 10K: requires siege tactics / air power (ww1)

  // min for 10km tile is 3K. otherwise its open. 08-12K is good. 60% loss = rout

  // required to cover an area is based on terrain (flat, floodplane, hill) and features (cities/woods/river)
  // assume 10km tile
  const terrainTable: Record<string, number> = {
    flat:        1.0,
    floodplain:  1.3,
    hills:       1.5,
    woods:       2.0,
    river:       2.5,
    trenches:    2.0,
    urban:       3.5,
    fortified:   5.0,
  };


/// ai start
  // garrison quality — how well a tile is held based on manpower present
  // below token: practically undefended, attacker rolls through
  // token: observation posts, bypassed in hours
  // thin: delaying defense, holds a day
  // moderate: real defense but gaps exploitable
  // adequate: doctrinal standard, needs 3:1 to break
  // strong: fortified sector, needs overwhelming force
  const garrisonThresholds = {
    token:      500,   //   50/km — a few outposts, bypassed without fighting
    screen:    1500,   //  150/km — recon screen, delays hours not days
    thin:      3000,   //  300/km — minimum real defense, gaps between positions
    moderate:  5000,   //  500/km — solid line but no reserve. one breach = collapse
    adequate:  8000,   //  800/km — doctrinal standard, 2 echelons + local reserve
    strong:   12000,   // 1200/km — dense defense, normandy hedgerow / prepared line
    fortress: 15000,   // 1500/km — kursk density, defense in depth with reserves
  };

  function garrisonFactor(manpower: number): number {
    if (manpower < garrisonThresholds.token)    return 0.0;  // undefended, walk through
    if (manpower < garrisonThresholds.screen)   return 0.1;  // tripwire only
    if (manpower < garrisonThresholds.thin)     return 0.3;  // delaying action
    if (manpower < garrisonThresholds.moderate) return 0.5;  // holds probes, not attacks
    if (manpower < garrisonThresholds.adequate) return 0.75; // real defense, exploitable gaps
    if (manpower < garrisonThresholds.strong)   return 1.0;  // full defensive capability
    if (manpower < garrisonThresholds.fortress) return 1.15; // dense, hard to crack
    return 1.3;                                              // kursk-level, needs overwhelming force
  }

  // effective defense multiplier = terrain * garrison quality
  // e.g. 3000 men in woods: 2.0 * 0.4 = 0.8x — woods help but not enough men to use it
  //      12000 men in woods: 2.0 * 1.0 = 2.0x — full benefit
  //      1000 men in fortified: 5.0 * 0.1 = 0.5x — bunkers don't man themselves
/// ai stop 


  /// ok if full recon.
  
  if (defender.manpower < garrisonThresholds.thin) {
    // bypassed in hours
    // we probe during day
    // we wait untill night
    // we sneak behind enemies.
    // then we fight them. effectively meaning defenders are at a disadvantage.

    /// attacking 10k men into 2k men 
    /// recon attacks happens first
    /// at night we roll to go behind positions or attack a few. 
    /// then the enemy is penaltized. 
    /// so before breakthrough attacker at disadvantage.
    /// after breakthrough attacker at advantage.

    /// without recon commander commits reservers.  when outmatched 
  }

  // kursk example: 16 km ^ 2 = 1 corps. 8 km ^ 2 div. 4 km ^ 2 reg. 2 km ^ 2 bat. 1 km ^ 1 comp.
  // so this is the factor of triangle. 
  // Companies Front. 1 comp. 2 bat. 4 reg. 8 div. 16 corps. 
  // Companies Equipment. 1 comp. 3 bat. 9 reg. 27 div. 81 corops.

  // so for a tradeoff. smaller tiles are like more micromanagement for a real scale war.
  // but less for a bigger war. On the other hand you are expeceted that your battles will be automated by the ai with CV only.

  // i would like a duel setup. that uses companies as the main level of units. instead of batt like hoi4
  // the wite 2 setup is a bit wierd. i like the range ammo abstraction
  // so i would like to try and expand on that with more ideas. with direct dice rolls for each company * strength

  // the further down i go in abstraction the more odds are easier to manage. And requires better AI i guess. 
  // So a squad fightning another squad can more become  1:1 victory.
  // but div would have to be 2:1


  // Battalion semi sufficient.
  // Regiment smallest self sufficinet. unit. it has arty, rifle, and anti tank.
  // so each company attacks...

  // so do i have an idea??? maybe. 
  // the attacker tries to attack with each company. the defender rolls a dice to see if they defend
  // the attackere wins often if they are 2:1 
  
  // so the idea is that the game stores each company as state.
  // adn then they get reinforceed. but maybe i should go more abstract like other war games. 
  // that means div level of complexity?

  // so instead of doing dice roll for each attack on  atrench. to be a simple dice roll for each regiment instead
  

  // suppression logic. 1 105 battery. disrupted. 2 batteries suppressed. 4 batteries pinned. 8 batteries stunned.
  // ok. you have a regiment attacking another regiment. 3 battalions. So 3 unique targets.
  // ai generated scenario
  // I/II/III 394th vs I/II/III 1028th
  // 394th has art bat. with 3 batteries of 12 med guns + 12 heavy guns. 1028th has 8 guns left.
  // I/1028 - defending village. II/1028 - defending hilltop east. III/1028 - defending woods south 


  // RECON: 0500 0700
  // I/394 recons village (recon platoon). draws MG fire. Finds 2 strongpoints. loses 8 
  // II/394 spots trench. finds gap. lose 3 men
  // III/394 moves through woods. Ambushed lose 23. attacks 6 
  // 394 found I/1028 II/1028. 1 gap. 1 strongpoint

  // 0700 0800 ARTILLERY
  // Planning. Strong point. 4 Batteries. Trenches 2 Batteries. Woods 2 Batteries
  // I/1028th lose 40 rifle. 1mg permanent. suppressed. Half effective. 1hr 
  // II/1028th lose 25 rifle. half suppress (trench). 25% effecctive.
  // III/1028th lose 10. Random fire. No suppression (hidden). 
  
  // 0800 1100 ASSAULT
  // Planning. Main force attacks gap. 
  // II/394 attacks hill gap with support
  // III/394 suppresses village
  // I/394 also attacks gap north??? how does this make sense?


  // MAIN ATTACK 
  // II/1028 (675 vs 1650) 2.4 : 1. 

  // lets assume a hex has 12 features. villages/forrests/fields/hills.
  // flatland: 4 villages 4 fields 2 hills 2 forrests.
  // city: 8 villages 1 hill 2 forrests 1 field
  // hill: 2 villages 2 fields 6 hills 2 forrest
  // light woods: 4 villages 4 forrests 2 fields 2 hills
  // heavy woods: 2 villages 8 forrests 2 hills

  // ok for this regiment battle we assume its a 1v1 on a 2|3|2 hex grid.
  // ai automatically handles the hex positioning. 
  // 50/50 either front or middle row. scouts has to detect where the enemy is.  
  // then artillery suppresses each tile. 


  // 0500 scout round. find positions and determine attacks
  //   I/394 recon tile 01
  //  II/394 recon tile 02
  // III/394 recon tile 03
  // outcomes. ambush. attacked. scouted. weakpoints. strongpoints

  // 0700 artillery round.  suppression
  // outcomes. suppressed. pinned. stunned. no effect. destroyed
  
  // 0900 assault round. take ground.
  //   I/394 attacks tile 11.
  //  II/394 attacks tile 12
  // III/394 supports tile 11.
  // outcomes. defenders held. defenders retreat. defenders rout. defenders counterattack. 
  // modifiers. flanks. ambushed.

  // battalion commander moves 
  // does that make sense? 
  
  // so order goes like this 
  // recon -> art ->  attacker move, defender react
  // 

  // ww2 casual data:
  // Attacking infantry in the open: 2-4% of engaged strength per day
  // Attacking infantry with fire support: 1-3%
  // Defending infantry in prepared positions: 0.5-1.5%
  // Defending infantry in the open: 2-3% (artillery eats you)
  // Urban combat (boh sides): 3-6%  

  // we also have position (3-6 km) /  zone (10-20km)  attack / defense.

  return { attacker, defender, rounds };
}

// ═════════════════════════════════════════════════════════
//  Rendering
// ═════════════════════════════════════════════════════════
let battleData: BattleData;

function runBattle(): void {
  const atkTemplate = DIVISIONS.panzer_division;
  const defTemplate = DIVISIONS.soviet_rifle_1944;
  const result = simulate(clone(atkTemplate), clone(defTemplate));

  battleData = {
    attacker: { division: atkTemplate.name, before: clone(atkTemplate), after: result.attacker, cv_before: computeCV(atkTemplate), cv_after: computeCV(result.attacker) },
    defender: { division: defTemplate.name, before: clone(defTemplate), after: result.defender, cv_before: computeCV(defTemplate), cv_after: computeCV(result.defender) },
    rounds: result.rounds,
  };
  render(battleData);
}

function render(d: BattleData): void {
  el("atk-name").textContent = d.attacker.division;
  el("def-name").textContent = d.defender.division;
  el("atk-cv").textContent = d.attacker.cv_before + "  →  " + d.attacker.cv_after;
  el("def-cv").textContent = d.defender.cv_before + "  →  " + d.defender.cv_after;

  const atkWins = d.attacker.cv_after > d.defender.cv_after;
  el("verdict").textContent = atkWins ? "Axis Victory" : "Soviet Victory";
  el("winner").textContent  = atkWins ? d.attacker.division : d.defender.division;
  el("score").textContent   = d.attacker.cv_after + " : " + d.defender.cv_after;

  renderUnits("atk-units", d.attacker.before, d.attacker.after);
  renderUnits("def-units", d.defender.before, d.defender.after);
  renderRounds(d.rounds);
}

function renderUnits(id: string, before: Division, after: Division): void {
  const tbody = el(id);
  tbody.innerHTML = "";
  for (const [key, label] of UNIT_KEYS) {
    const b = before[key] as number, a = after[key] as number;
    const tr = document.createElement("tr");
    tr.innerHTML =
      '<td class="px-2 py-1 text-neutral-800">' + esc(label) + "</td>" +
      '<td class="num text-right px-2 py-1 text-neutral-600">' + b + "</td>" +
      '<td class="num text-right px-2 py-1 text-neutral-600">' + a + "</td>" +
      '<td class="num text-right px-2 py-1 text-red-700 font-semibold">-' + Math.max(0, b - a) + "</td>";
    tbody.appendChild(tr);
  }
}

function renderRounds(rounds: RoundEntry[]): void {
  const tbody = el("rounds");
  tbody.innerHTML = "";
  rounds.forEach((r, i) => {
    const tr = document.createElement("tr");
    tr.className = "cursor-pointer";
    tr.innerHTML =
      '<td class="px-2 py-1 font-semibold text-purple-900">#' + (i + 1) + " " + esc(r.phase) + "</td>" +
      '<td class="px-2 py-1 text-blue-900">' + esc(r.attacker_action) + "</td>" +
      '<td class="px-2 py-1 text-red-900">' + esc(r.defender_action) + "</td>" +
      '<td class="num text-right px-2 py-1 text-red-700 font-semibold">' + r.attacker_losses + "</td>" +
      '<td class="num text-right px-2 py-1 text-red-700 font-semibold">' + r.defender_losses + "</td>";
    tr.addEventListener("click", () => showPopup(i));
    tbody.appendChild(tr);
  });
}

// ── Popup ───────────────────────────────────────────────
function showPopup(idx: number): void {
  const r = battleData.rounds[idx];
  el("popup-title").textContent     = "Round #" + (idx + 1) + " — " + r.phase;
  el("popup-atk-action").textContent = r.attacker_action;
  el("popup-def-action").textContent = r.defender_action;
  el("popup-atk-loss").textContent   = "Atk  -" + r.attacker_losses;
  el("popup-def-loss").textContent   = "Def  -" + r.defender_losses;
  el("popup-overlay").classList.remove("hidden");
  el("popup-overlay").classList.add("open");
}

el("popup-overlay").addEventListener("click", () => {
  el("popup-overlay").classList.add("hidden");
  el("popup-overlay").classList.remove("open");
});

// ── Toggle ──────────────────────────────────────────────
let detailsOpen = false;
el("toggle-bar").addEventListener("click", () => {
  detailsOpen = !detailsOpen;
  el("details").classList.toggle("hidden", !detailsOpen);
  el("details").classList.toggle("open", detailsOpen);
  el("toggle-bar").textContent = detailsOpen ? "[-] Details" : "[+] Details";
});

// ── Init ────────────────────────────────────────────────
runBattle();
