# Companion Checklist: ASM Story-Flag Invariants -> C Touchpoints

Companion to:
- `docs/asm_event_flag_structure_start_to_champion.md`

Purpose:
- give an exact, no-guessing map from each ASM progression invariant to the C files/functions we should inspect first tonight.

Status legend:
- `implemented`: behavior appears wired and reachable.
- `partial`: some of the chain exists, but at least one required edge is missing.
- `missing`: constants/data exist but no runtime story script path currently drives them.

## P0 Queue (Investigate First)

### 1) Route 22 rival chain must support BOTH battles (arm -> consume -> disarm)
- ASM reference:
  - `pokered-master/scripts/OaksLab.asm:636-638` (arm 1st, clear 2nd, set wants-battle)
  - `pokered-master/scripts/Route22.asm:59,72,74` (gate + variant select)
  - `pokered-master/scripts/Route22.asm:167,231` (1st battle beat + disarm)
  - `pokered-master/scripts/Route22.asm:326,375` (2nd battle beat + disarm)
- C touchpoints:
  - `src/game/oakslab_scripts.c:1290-1292`
  - `src/game/route22_scripts.c:133-135,185,265-266`
  - `src/data/event_constants.h:318-322`
- Status: `partial`
- Concrete gap:
  - `route22_scripts.c` only checks first-battle arm (`EVENT_1ST_ROUTE22_RIVAL_BATTLE`).
  - `EVENT_2ND_ROUTE22_RIVAL_BATTLE` and `EVENT_BEAT_ROUTE22_RIVAL_2ND_BATTLE` are defined, but not consumed/set by the Route 22 runtime script path.
  - Search confirms no runtime usage of `EVENT_BEAT_ROUTE22_RIVAL_2ND_BATTLE` outside constants/name tables.
- Why key:
  - this is a direct main-story rival chain and a common source of progression misfires.

### 2) Route 23 guards must enforce badge checks via BOTH badges and pass-flags
- ASM reference:
  - `pokered-master/scripts/Route23.asm:202-204` (`wObtainedBadges` test)
  - `pokered-master/scripts/Route23.asm:219` (set passed-* check flag)
- C touchpoints:
  - Guard logic functions exist: `src/game/gate_scripts.c:401-462` (`Gate_Route23_*`)
  - Map load hide logic exists: `src/game/gate_scripts.c:55-63`
  - Route23 NPC data: `src/data/event_data.c:834-842`
  - Route23 map entry: `src/data/event_data.c:3885`
- Status: `partial` (logic exists, wiring missing)
- Concrete gap:
  - `Gate_Route23_*` functions are defined but have no call sites outside `gate_scripts.c`/header.
  - Route23 NPC entries currently use `script=NULL`, so A-press interaction does not invoke gate logic.
- Why key:
  - this can silently allow or block League access incorrectly even if flags look right.

### 3) Elite Four -> Champion -> Hall of Fame event chain (including reset) must exist
- ASM reference:
  - `pokered-master/scripts/LancesRoom.asm:16,55,71,156`
  - `pokered-master/scripts/ChampionsRoom.asm:98,253`
  - `pokered-master/scripts/HallOfFame.asm:45` (`ResetEventRange INDIGO_PLATEAU_EVENTS_START, INDIGO_PLATEAU_EVENTS_END, 1`)
- C touchpoints:
  - Constants present: `src/data/event_constants.h:484-493`
  - Elite/Champion/HOF map data exists as NPC text/warps only:
    - `src/data/event_data.c:3824-3848` (Lorelei/Bruno/Agatha/Lance NPC blocks)
    - `src/data/event_data.c:3956-3971` (map entries for elite/champion/hof maps)
    - `src/data/event_data.c:2102-2121` (HOF + Champion NPC blocks)
- Status: `missing`
- Concrete gap:
  - No runtime references in `src/game` to `EVENT_BEAT_LANCE`, `EVENT_LANCES_ROOM_LOCK_DOOR`, `EVENT_BEAT_CHAMPION_RIVAL`, or elite-room autowalk flags.
  - No C equivalent for Indigo range reset (no `INDIGO_PLATEAU_EVENTS_START/END` defines or reset routine).
- Why key:
  - this is the terminal story chain; missing here blocks parity from late-game onward.

## P1 Queue (Next)

### 4) Viridian Gym aftermath should arm Route22 rival #2 in story flow
- ASM reference:
  - `pokered-master/scripts/ViridianGym.asm:167` sets `EVENT_2ND_ROUTE22_RIVAL_BATTLE` + `EVENT_ROUTE22_RIVAL_WANTS_BATTLE`
- C touchpoints:
  - Viridian gym related constants: `src/data/event_constants.h:23-24,319,322`
  - Viridian gym map data: `src/data/event_data.c:1051-1062,3896`
- Status: `missing`
- Concrete gap:
  - no runtime script writes `EVENT_BEAT_VIRIDIAN_GYM_GIOVANNI`, `EVENT_GOT_TM27`, or Route22 second-battle arm flags.

### 5) Story-special battle routing must remain isolated from generic trainer defeat marking
- ASM intent:
  - story battles set specific flags at scripted edges; normal trainers set their own defeat flag.
- C touchpoints:
  - central routing: `src/game/game.c:654-697`
  - generic trainer flag write: `src/game/trainer_sight.c:399-406`
  - special consumers:
    - `src/game/route22_scripts.c:178-197`
    - `src/game/cerulean_scripts.c:269-321`
    - `src/game/ss_anne_scripts.c:372-390`
- Status: `implemented` (for currently ported special arcs)
- Inspect for regressions:
  - ensure every special story battle has consume/onvictory/defeat hooks, or it will fall through to `Trainer_MarkCurrentDefeated()`.

### 6) Map-script-state parity (flags + per-map state machine)
- ASM intent:
  - progression uses both event flags and `w<Map>CurScript` transitions.
- C touchpoints:
  - active map script modules called on load: `src/game/game.c:103-117`
  - active script ticks called each frame: `src/game/game.c:886-1186`
- Status: `partial`
- Concrete gap:
  - only a subset of story maps currently has dedicated script modules wired in `game.c`.
  - Elite/Champion/Route23-specific state machines are not present as dedicated runtime modules.

## P2 Queue (Data-Model Gaps)

### 7) Trainer coverage in `event_data.c` is sparse vs full story needs
- Evidence:
  - defined trainer arrays currently include only a small subset (`kTrainers_PalletTown`, `Route3`, `Route6`, `Route24`, `Route25`, `ViridianForest`, `PewterGym`, `MtMoon1F`, `MtMoonB2F`, `CeruleanGym`, SS Anne rooms).
  - Elite/Champion/Viridian Gym/Route23 guards do not have `map_trainer_t` entries in current data layout.
- Status: `partial`
- Why it matters:
  - generic trainer sight/defeat flow cannot carry story-only elite/champion logic by itself.

## Practical Order For Tonight

1. Close Route 22 second-battle chain (arm/select/beat/disarm parity).
2. Wire Route23 guard callbacks from map NPC interactions to `Gate_Route23_*`.
3. Add minimal elite/champion story runtime module(s):
   - Lance room lock + defeat flag
   - Champion rival beat flag
   - HallOfFame Indigo-range reset equivalent
4. Backfill Viridian Gym -> Route22 second-battle arm edge.
5. Re-run full start->champion flag trace against ASM.

## Quick Verification Queries

```powershell
# second Route22 story flags: should appear in runtime, not just constants
Get-ChildItem -Recurse -File src | Select-String -Pattern "EVENT_2ND_ROUTE22_RIVAL_BATTLE|EVENT_BEAT_ROUTE22_RIVAL_2ND_BATTLE"

# Route23 guard wiring: Gate_Route23_* should have call sites beyond gate_scripts.c/.h
Get-ChildItem -Recurse -File src | Select-String -Pattern "Gate_Route23_Earth\(|Gate_Route23_Volcano\(|Gate_Route23_Cascade\("

# elite/champion runtime usage
Get-ChildItem -Recurse -File src\game | Select-String -Pattern "EVENT_BEAT_CHAMPION_RIVAL|EVENT_BEAT_LANCE|EVENT_LANCES_ROOM_LOCK_DOOR"
```
