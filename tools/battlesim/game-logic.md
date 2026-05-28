# Battles — Game Logic

## Overview

Regiment-level hex wargame: **394th Infantry Rgt** (attacker) vs **1028th Rifle Rgt** (defender).
4 × 10 flat-top hex grid, 30-minute turns starting at 14:00. Nightfall ends the battle at 20:00.

## Forces

### Attacker (394th)
- **3 battalions** (I, II, III), each with **3 infantry companies**
  - 2 assault companies (start rows G–H)
  - 1 reserve company (start row I, committed later)
- **1 support platoon** (SPT) — 4 mortars, 2 AT guns — follows I Bn lead company
- **Off-map artillery**: 12 × 105mm guns

### Defender (1028th)
- **3 battalions** (I, II, III), each with **3 infantry companies**
  - 4 companies on **row B** (main defense line — full blocking line)
  - 2 companies on **row C** (forward screen, best-cover tiles)
  - 3 companies on **row A** (reserve, 1 per battalion)
- All frontline companies start **entrenched** (×1.3 defense bonus)
- **Off-map artillery**: 8 × 76mm guns

### Company Stats
| Stat    | Value | Notes                            |
|---------|-------|----------------------------------|
| Men     | 220   | Strength                         |
| Rifles  | 180   | Main firepower                   |
| MG      | 3     | Machine guns                     |
| Mortar  | 2     | Organic mortars                  |
| AT      | 0–1   | Anti-tank guns (defenders only)  |
| Morale  | 75–85 | Breaks below 15–25               |

## Turn Sequence (each 30-min turn)

1. **Reserve commit check** — see below
2. **Attacker artillery** — fires on all revealed defender tiles
3. **Counter-battery** — chance to destroy opposing guns
4. **Movement** — attackers advance toward objectives
5. **Assaults** — companies attack adjacent defenders
6. **Defender artillery** — interdiction fire on random attacker tile
7. **Ownership & fog update** — tile control, unit spotting
8. **Phase objective check** — consolidate, advance to next phase

## Phased Attack System

Battalions attack in structured phases, not a continuous push:

### Phase 1 — Forward Screen (Row C)
Each assault company gets a specific **objective tile** on row C.
Companies advance, assault, and **stop when their objective is taken**.

### Phase 2 — Main Defense Line (Row B)
Triggered only when **all Phase 1 objectives are secured** (or assigning companies routed).
New objectives assigned on row B. Companies resume the attack.

### Phase 3 — Breakthrough (Row A)
Same pattern. Reaching row A = attacker victory.

### Holding Behavior
- A company that takes its objective **consolidates** — it stops advancing.
- Consolidated companies still **fire support** adjacent friendlies in combat.
- Consolidated companies **defend against counterattacks**.
- No company advances past its objective until **all objectives in the phase are complete**.

## Movement

- **2 hex movement** normally per turn
- **1 hex** if starting in enemy Zone of Control (adjacent to revealed enemy)
- Entering enemy ZOC **ends movement** immediately
- **Stacking limit**: 1 company per hex (2 in forest/ridge terrain)
- Reserve companies **hold position** until committed
- Support platoon **follows** its parent battalion's lead company

## Combat

### Assault Resolution
- **Combat Value (CV)** = rifles/10 + MG + mortar×0.67 + AT×1.5
- Attacker CV vs Defender CV ratio, modified by:
  - **Terrain multiplier** (field ×1.0, hill ×1.5, village ×1.8, forest ×2.0, ridge ×1.7)
  - **Entrenchment** (×1.3 bonus for dug-in defenders)
  - **Flanking assault** (×1.2 bonus if attacker attacks from same row)
  - **Flanking fire** — adjacent non-attacked defenders add 30% of their CV
  - **Suppression** — reduces defender effectiveness

### Pinning
- Assault planning **pins every defender** by assigning at least one attacker to each
- Pinned defenders cannot provide flanking fire to neighbors
- Remaining attackers concentrate on the **weakest** defender for breakthrough
- **Both flanks must attack** — if defenders aren't pinned, they support each other

### Casualty Rates (per 30-min turn)
| Effectiveness | Attacker Loss | Defender Loss |
|---------------|--------------|--------------|
| ≥ 3.0 (overwhelming) | 0.5% | 4% |
| ≥ 2.0 (strong) | 1% | 3% |
| ≥ 1.5 (favorable) | 2% | 2% |
| ≥ 1.0 (parity) | 3% | 1.2% |
| < 1.0 (unfavorable) | 4% | 0.6% |

### Routing
- Company routs if **morale ≤ 15** or **men < 80**
- Defender retreats if **morale < 35** (falls back one row, loses entrenchment)
- Defender retreats to rear tile if **morale ≤ 25** or **men < 100** during combat

### Counterattacks
Defenders counterattack when:
- Not already in contact with enemy on own tile
- Adjacent attacker-occupied tile available
- Own CV ≥ 0.9 × enemy CV on target tile
- Own morale ≥ 50
- Priority: captured tiles, isolated units, concentrated counter-blows

## Reserve Commitment

### Attacker Reserves (3rd company of each bn)
Committed when any of:
- 2+ forward companies routed
- All forward companies engaged AND turn ≥ 3
- Turn ≥ 6 (3 hours elapsed)

### Defender Reserves (3rd company of each bn)
Committed individually when their battalion's forward companies are in contact with the enemy.

## Artillery

### Attacker (12 × 105mm)
- Fires on **all revealed** defender tiles, guns split evenly
- Causes casualties + suppression (DISRUPTED → SUPPRESSED → PINNED)
- Morale damage per barrage

### Defender (8 × 76mm)
- Fires on a **random** attacker-occupied tile
- Lower damage, no suppression tracking

### Counter-battery
Each turn: 33% chance each side loses 1 gun.

## Fog of War

- **Terrain is always visible** — you can see terrain type on every hex
- **Enemy units are hidden** until spotted
- Spotting: automatic when adjacent, roll-based at range (forest/village harder to spot)
- Once spotted, units stay revealed (no re-fog)
- Hidden defenders don't project Zone of Control

## Victory Conditions

| Condition | Result |
|-----------|--------|
| Attacker reaches row A | **Attacker Victory** |
| All defenders routed | **Attacker Victory** |
| All attackers routed | **Defender Victory** |
| Turn 12 (20:00) reached | **Stalemate** |
