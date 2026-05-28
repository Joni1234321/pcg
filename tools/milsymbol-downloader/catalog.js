/* milsymbol catalog -- curated common symbols (2525C / APP-6B 15-char SIDCs).
 *
 * Each entry's SIDC has affiliation byte at position 1 (0-indexed) set to 'F'
 * (friend) by default. The UI swaps that byte based on the affiliation
 * selector (F=friend, H=hostile, N=neutral, U=unknown, P=pending, A=assumed
 * friend, S=suspect, J=joker, K=faker).
 *
 * Structure: { category, name, sidc }
 */
window.MILSYMBOL_CATALOG = [
    // ─── Land - Units (S*GPU...) ───────────────────────────────────────────
    { c: "Land / Infantry",  n: "Infantry",                       s: "SFGPUCI-----E" },
    { c: "Land / Infantry",  n: "Mechanized Infantry",            s: "SFGPUCIZ----E" },
    { c: "Land / Infantry",  n: "Motorized Infantry",             s: "SFGPUCIM----E" },
    { c: "Land / Infantry",  n: "Airborne Infantry",              s: "SFGPUCIA----E" },
    { c: "Land / Infantry",  n: "Air Assault Infantry",           s: "SFGPUCIL----E" },
    { c: "Land / Infantry",  n: "Light Infantry",                 s: "SFGPUCIS----E" },
    { c: "Land / Infantry",  n: "Mountain Infantry",              s: "SFGPUCIO----E" },
    { c: "Land / Infantry",  n: "Naval Infantry (Marines)",       s: "SFGPUCIN----E" },

    { c: "Land / Armor",     n: "Armor",                          s: "SFGPUCA-----E" },
    { c: "Land / Armor",     n: "Armor, Heavy",                   s: "SFGPUCAH----E" },
    { c: "Land / Armor",     n: "Armor, Light",                   s: "SFGPUCAL----E" },
    { c: "Land / Armor",     n: "Armored Recon",                  s: "SFGPUCAA----E" },
    { c: "Land / Armor",     n: "Armored Amphibious",             s: "SFGPUCAW----E" },

    { c: "Land / Recon",     n: "Reconnaissance",                 s: "SFGPUCR-----E" },
    { c: "Land / Recon",     n: "Reconnaissance, Cavalry",        s: "SFGPUCRV----E" },
    { c: "Land / Recon",     n: "Reconnaissance, Long Range",     s: "SFGPUCRL----E" },

    { c: "Land / Artillery", n: "Field Artillery",                s: "SFGPUCF-----E" },
    { c: "Land / Artillery", n: "Self-Propelled Artillery",       s: "SFGPUCFHE---E" },
    { c: "Land / Artillery", n: "Towed Artillery",                s: "SFGPUCFHA---E" },
    { c: "Land / Artillery", n: "Rocket Artillery (MLRS)",        s: "SFGPUCFR----E" },
    { c: "Land / Artillery", n: "Mortar",                         s: "SFGPUCFM----E" },
    { c: "Land / Artillery", n: "Air Defense Artillery",          s: "SFGPUCD-----E" },
    { c: "Land / Artillery", n: "AD Missile (SAM)",               s: "SFGPUCDM----E" },

    { c: "Land / Support",   n: "Engineer",                       s: "SFGPUCE-----E" },
    { c: "Land / Support",   n: "Combat Engineer",                s: "SFGPUCEC----E" },
    { c: "Land / Support",   n: "Signal",                         s: "SFGPUUS-----E" },
    { c: "Land / Support",   n: "Medical",                        s: "SFGPUUM-----E" },
    { c: "Land / Support",   n: "Supply / Logistics",             s: "SFGPUSS-----E" },
    { c: "Land / Support",   n: "Maintenance",                    s: "SFGPUSM-----E" },
    { c: "Land / Support",   n: "Transportation",                 s: "SFGPUST-----E" },
    { c: "Land / Support",   n: "Military Police",                s: "SFGPUUL-----E" },
    { c: "Land / Support",   n: "Headquarters",                   s: "SFGPUH------E" },

    { c: "Land / Special",   n: "Special Forces",                 s: "SFGPUCIZ----E" },
    { c: "Land / Special",   n: "Sniper",                         s: "SFGPUCIS----E" },
    { c: "Land / Special",   n: "Anti-Tank",                      s: "SFGPUCAT----E" },

    { c: "Land / Equipment", n: "Tank",                           s: "SFGPEWA-----E" },
    { c: "Land / Equipment", n: "Armored Personnel Carrier",      s: "SFGPEVAA----E" },
    { c: "Land / Equipment", n: "Infantry Fighting Vehicle",      s: "SFGPEVAI----E" },
    { c: "Land / Equipment", n: "Truck",                          s: "SFGPEVU-----E" },
    { c: "Land / Equipment", n: "Radar",                          s: "SFGPEWRR----E" },

    // ─── Air (S*A...) ──────────────────────────────────────────────────────
    { c: "Air",              n: "Fixed-Wing Aircraft",            s: "SFAPMF------E" },
    { c: "Air",              n: "Fighter",                        s: "SFAPMFF-----E" },
    { c: "Air",              n: "Bomber",                         s: "SFAPMFB-----E" },
    { c: "Air",              n: "Attack/Strike",                  s: "SFAPMFA-----E" },
    { c: "Air",              n: "Cargo / Transport",              s: "SFAPMFC-----E" },
    { c: "Air",              n: "Tanker (Refueling)",             s: "SFAPMFK-----E" },
    { c: "Air",              n: "Reconnaissance Aircraft",        s: "SFAPMFR-----E" },
    { c: "Air",              n: "Helicopter",                     s: "SFAPMH------E" },
    { c: "Air",              n: "Attack Helicopter",              s: "SFAPMHA-----E" },
    { c: "Air",              n: "Utility Helicopter",             s: "SFAPMHU-----E" },
    { c: "Air",              n: "Cargo Helicopter",               s: "SFAPMHC-----E" },
    { c: "Air",              n: "UAV / Drone",                    s: "SFAPMFQ-----E" },
    { c: "Air",              n: "Missile (in flight)",            s: "SFAPWMS-----E" },

    // ─── Sea Surface (S*S...) ──────────────────────────────────────────────
    { c: "Sea",              n: "Combatant Line, Carrier",        s: "SFSPCLCV----E" },
    { c: "Sea",              n: "Battleship",                     s: "SFSPCLBB----E" },
    { c: "Sea",              n: "Cruiser",                        s: "SFSPCLCC----E" },
    { c: "Sea",              n: "Destroyer",                      s: "SFSPCLDD----E" },
    { c: "Sea",              n: "Frigate",                        s: "SFSPCLFF----E" },
    { c: "Sea",              n: "Patrol Boat",                    s: "SFSPCLLL----E" },
    { c: "Sea",              n: "Amphibious Warfare Ship",        s: "SFSPCA------E" },

    // ─── Subsurface (S*U...) ───────────────────────────────────────────────
    { c: "Subsurface",       n: "Submarine",                      s: "SFUPS-------E" },
    { c: "Subsurface",       n: "Attack Submarine",               s: "SFUPSF------E" },
    { c: "Subsurface",       n: "Ballistic Missile Sub",          s: "SFUPSB------E" },

    // ─── Installations (S*GPI...) ──────────────────────────────────────────
    { c: "Installation",     n: "Military Base / Installation",   s: "SFGPIB------E" },
    { c: "Installation",     n: "Airport / Air Base",             s: "SFGPIBA-----E" },
    { c: "Installation",     n: "Ammunition Cache",               s: "SFGPIRM-----E" },
    { c: "Installation",     n: "Fuel Storage",                   s: "SFGPIRP-----E" },
    { c: "Installation",     n: "Bridge",                         s: "SFGPIRR-----E" },
    { c: "Installation",     n: "Hospital",                       s: "SFGPIBM-----E" },
];
