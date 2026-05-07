# ASM Event Flag Structure (Start -> Champion)

This document is a direct reference of how the **original ASM** uses event flags for main-story progression from game start to Champion/Hall of Fame.

Scope rules:
- Only facts traced from `pokered-master` ASM files.
- No inferred C-port behavior.
- Focused on progression-critical events and how scripts gate/arm/reset them.

## 1) Core Event-Flag Architecture

### Storage model
- Event flags live in WRAM as a bit array:
  - `pokered-master/ram/wram.asm:2143`
  - `wEventFlags:: flag_array NUM_EVENTS`
- Event IDs are defined in a single ordered list:
  - `pokered-master/constants/event_constants.asm`

### Access model
- Script macros wrap direct bit operations:
  - `pokered-master/macros/scripts/events.asm`
  - `CheckEvent`, `SetEvent`, `ResetEvent`, plural/range variants (`SetEvents`, `ResetEvents`, `SetEventRange`, `ResetEventRange`), and check-and-set/reset helpers.
- Generic runtime bit API is centralized in:
  - `pokered-master/engine/flag_action.asm`
  - `FlagActionPredef` / `FlagAction` with action modes (`FLAG_TEST`, `FLAG_SET`, `FLAG_RESET`).

### Important structural reality
- Story flow is **not flags-only**. Most maps combine:
  - event flags, and
  - local script state variables (`w<MapName>CurScript`) plus map-script dispatch tables.
- Result: a translation can match flags but still diverge if script-state transitions differ.

## 2) Start-of-Game Chain (Pallet/Oak/Lab/Mart)

### Pallet -> Oak escort trigger
- `PalletTown` checks/sets escort state:
  - `scripts/PalletTown.asm:22` checks `EVENT_FOLLOWED_OAK_INTO_LAB`
  - `scripts/PalletTown.asm:39` sets `EVENT_OAK_APPEARED_IN_PALLET`

### Oak's Lab initial progression
- Escort completion:
  - `scripts/OaksLab.asm:112` sets `EVENT_FOLLOWED_OAK_INTO_LAB`
  - `scripts/OaksLab.asm:113` sets `EVENT_FOLLOWED_OAK_INTO_LAB_2`
- Starter flow:
  - `scripts/OaksLab.asm:146` sets `EVENT_OAK_ASKED_TO_CHOOSE_MON`
  - `scripts/OaksLab.asm:335` sets `EVENT_GOT_STARTER`
  - `scripts/OaksLab.asm:433` sets `EVENT_BATTLED_RIVAL_IN_OAKS_LAB`
- Pokedex + parcel handoff state:
  - `scripts/OaksLab.asm:600` sets `EVENT_GOT_POKEDEX`
  - `scripts/OaksLab.asm:601` sets `EVENT_OAK_GOT_PARCEL`

### Oak arms Route 22 rival #1 at this point
- `scripts/OaksLab.asm:636` sets `EVENT_1ST_ROUTE22_RIVAL_BATTLE`
- `scripts/OaksLab.asm:637` resets `EVENT_2ND_ROUTE22_RIVAL_BATTLE`
- `scripts/OaksLab.asm:638` sets `EVENT_ROUTE22_RIVAL_WANTS_BATTLE`

### Poke Balls gift gate
- Oak gift is check-and-set guarded:
  - `scripts/OaksLab.asm:1023` `CheckAndSetEvent EVENT_GOT_POKEBALLS_FROM_OAK`
- This is a one-time gate and a progression signal consumed by Pallet/Lab logic.

### Viridian Mart parcel loop
- `scripts/ViridianMart.asm:9` checks `EVENT_OAK_GOT_PARCEL`
- `scripts/ViridianMart.asm:58` sets `EVENT_GOT_OAKS_PARCEL`

## 3) Rival Battle Flag Pattern (Critical)

This pattern repeats and is central to story stability:
- one flag arms visibility/trigger,
- one flag selects which encounter variant,
- one flag records completion,
- arm flags are reset after battle cleanup.

### Route 22 script gate
- `scripts/Route22.asm:59` requires `EVENT_ROUTE22_RIVAL_WANTS_BATTLE`
- Variant select:
  - `:72` check `EVENT_1ST_ROUTE22_RIVAL_BATTLE`
  - `:74` check `EVENT_2ND_ROUTE22_RIVAL_BATTLE`

### Route 22 first battle
- Post-win record:
  - `scripts/Route22.asm:167` sets `EVENT_BEAT_ROUTE22_RIVAL_1ST_BATTLE`
- Cleanup disarm:
  - `scripts/Route22.asm:231` resets `EVENT_1ST_ROUTE22_RIVAL_BATTLE` + `EVENT_ROUTE22_RIVAL_WANTS_BATTLE`

### Route 22 second battle
- Armed later from Viridian Gym Giovanni aftermath:
  - `scripts/ViridianGym.asm:167` sets `EVENT_2ND_ROUTE22_RIVAL_BATTLE` + `EVENT_ROUTE22_RIVAL_WANTS_BATTLE`
- Post-win record:
  - `scripts/Route22.asm:326` sets `EVENT_BEAT_ROUTE22_RIVAL_2ND_BATTLE`
- Cleanup disarm:
  - `scripts/Route22.asm:375` resets `EVENT_2ND_ROUTE22_RIVAL_BATTLE` + `EVENT_ROUTE22_RIVAL_WANTS_BATTLE`

### Rival text state checks
- `scripts/Route22.asm:388` checks `EVENT_BEAT_ROUTE22_RIVAL_1ST_BATTLE`
- `scripts/Route22.asm:401` checks `EVENT_BEAT_ROUTE22_RIVAL_2ND_BATTLE`

## 4) Mid-Story Mainline Flags (Selective, progression-critical)

### Cerulean rival + rocket thief
- `scripts/CeruleanCity.asm:175` sets `EVENT_BEAT_CERULEAN_RIVAL`
- `scripts/CeruleanCity.asm:29` sets `EVENT_BEAT_CERULEAN_ROCKET_THIEF`
- Map logic checks these for encounter/object progression (`:43`, `:64`, `:257`, `:288`).

### Bill / SS Anne progression
- `scripts/BillsHouse.asm:173` sets `EVENT_GOT_SS_TICKET`
- `scripts/SSAnneCaptainsRoom.asm:31` sets `EVENT_GOT_HM01`
- Dock departure chain:
  - `scripts/VermilionDock.asm:12` sets `EVENT_STARTED_WALKING_OUT_OF_DOCK`
  - `scripts/VermilionDock.asm:36` sets `EVENT_WALKED_OUT_OF_DOCK`
  - `scripts/VermilionDock.asm:40` sets `EVENT_SS_ANNE_LEFT`

### Tower / Silph / Giovanni progression
- `scripts/PokemonTower7F.asm:229` sets `EVENT_RESCUED_MR_FUJI`
- `scripts/SilphCo7F.asm:212` sets `EVENT_BEAT_SILPH_CO_RIVAL`
- `scripts/SilphCo11F.asm:233` sets `EVENT_BEAT_SILPH_CO_GIOVANNI`
- `scripts/ViridianGym.asm:142` sets `EVENT_BEAT_VIRIDIAN_GYM_GIOVANNI`

## 5) Route 23 + Indigo Entry Gate Model

### Badge checks are dual-source
Route 23 guard scripts use both:
- persistent pass flags (`EVENT_PASSED_*BADGE_CHECK`), and
- live badge ownership bits (`wObtainedBadges`).

Key sites:
- Event-flag addressing/bit ops:
  - `scripts/Route23.asm:53`, `:155`, `:161`, `:167`, `:173`, `:179`, `:185`, `:191`, `:219`
- Badge possession test:
  - `scripts/Route23.asm:202` reads `wObtainedBadges` via `FlagActionPredef`

Meaning:
- a guard can memoize that you passed, but still verifies badge ownership path through shared bit logic.

## 6) Elite Four -> Champion -> Hall of Fame

### Elite room progression flags
- Lorelei/Bruno/Agatha each have autowalk and trainer-beat flags in their room scripts:
  - `LoreleisRoom.asm`, `BrunosRoom.asm`, `AgathasRoom.asm`
- Lance room has explicit door-lock and completion:
  - `scripts/LancesRoom.asm:16` checks `EVENT_LANCES_ROOM_LOCK_DOOR`
  - `scripts/LancesRoom.asm:71` check-and-sets `EVENT_LANCES_ROOM_LOCK_DOOR`
  - `scripts/LancesRoom.asm:55` checks `EVENT_BEAT_LANCE`
  - `scripts/LancesRoom.asm:156` sets `EVENT_BEAT_LANCE`

### Champion battle completion
- `scripts/ChampionsRoom.asm:98` sets `EVENT_BEAT_CHAMPION_RIVAL`
- Champion text gating checks same flag at `:253`

### Hall of Fame reset behavior
- On Hall of Fame save script:
  - `scripts/HallOfFame.asm:45`
  - `ResetEventRange INDIGO_PLATEAU_EVENTS_START, INDIGO_PLATEAU_EVENTS_END, 1`
- `INDIGO_PLATEAU_EVENTS_START/END` range is defined in `constants/event_constants.asm` around the Elite/Champion events.

Meaning:
- Indigo/Elite/Champion temporary progression flags are intentionally batch-reset post-clear.
- Any C-port mismatch in this range or reset timing can create repeat/lockout anomalies.

## 7) What To Verify First When Diffing Story Scripts

When diffing ASM vs C for main story stability, prioritize these invariants:

1. **Arm -> consume -> disarm symmetry**
- Every rival/story trigger arm flag set should have a corresponding deterministic reset path after resolution.

2. **Beat flags set only on intended post-battle script edge**
- Ensure no non-story battle flow can write story beat flags.

3. **Map script-state alignment (`w*CurScript`)**
- Flag parity alone is insufficient if script IDs/state transitions diverge.

4. **Range reset parity for Indigo events**
- Confirm Champion/HallOfFame completion executes the same event range reset.

5. **Route23 dual-source badge gate parity**
- Preserve both event memoization and live `wObtainedBadges` checks.

## 8) Files to Keep Open While Working Tonight

- `pokered-master/constants/event_constants.asm`
- `pokered-master/macros/scripts/events.asm`
- `pokered-master/engine/flag_action.asm`
- `pokered-master/scripts/OaksLab.asm`
- `pokered-master/scripts/Route22.asm`
- `pokered-master/scripts/CeruleanCity.asm`
- `pokered-master/scripts/ViridianGym.asm`
- `pokered-master/scripts/Route23.asm`
- `pokered-master/scripts/LancesRoom.asm`
- `pokered-master/scripts/ChampionsRoom.asm`
- `pokered-master/scripts/HallOfFame.asm`
