# ASM Dependency Chain: Post–Lt. Surge to Champion

Scope:
- Starts at the moment Lt. Surge is beaten.
- Follows story-critical script structure/dependencies in ASM.
- Uses only `pokered-master/scripts/*.asm` facts (no C-port assumptions).

## 1) Lt. Surge Completion Node (Starting Point)

Source: `pokered-master/scripts/VermilionGym.asm`

- On Surge post-battle script:
  - `SetEvent EVENT_BEAT_LT_SURGE` (line 69)
  - Optional TM ownership event:
    - `SetEvent EVENT_GOT_TM24` if item is received (line 76)
  - Thunderbadge bit is written to both badge containers:
    - `wObtainedBadges` + `wBeatGymFlags` (lines 84, 86)
- Re-talk flow:
  - Checks `EVENT_BEAT_LT_SURGE` (line 116)
  - Checks `EVENT_GOT_TM24` for post-battle item branch (line 118)

This is the baseline state for all post-Surge progression.

## 2) Midgame Core Chain After Surge (Critical Story Gates)

### 2.1 Rocket Hideout completion (Celadon arc)

Source: `pokered-master/scripts/RocketHideoutB4F.asm`

- Giovanni defeat marks completion:
  - `SetEvent EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI` (line 54)
- Lift Key drop is guarded one-time:
  - `CheckAndSetEvent EVENT_ROCKET_DROPPED_LIFT_KEY` (line 193)
- Silph Scope is present as a pickup text slot:
  - `TEXT_ROCKETHIDEOUTB4F_SILPH_SCOPE` (line 85)

### 2.2 Pokémon Tower unlock chain

Source: `pokered-master/scripts/PokemonTower6F.asm`, `PokemonTower7F.asm`, `MrFujisHouse.asm`

- Tower 6F Marowak gate:
  - `CheckEvent EVENT_BEAT_GHOST_MAROWAK` (line 26)
  - Defeat sets `EVENT_BEAT_GHOST_MAROWAK` (line 64)
- Tower 7F Mr. Fuji rescue:
  - `SetEvent EVENT_RESCUED_MR_FUJI` (line 229)
  - `SetEvent EVENT_RESCUED_MR_FUJI_2` (line 230)
- Mr. Fuji house reward gate:
  - Checks `EVENT_GOT_POKE_FLUTE` (line 72)
  - Gives flute and sets `EVENT_GOT_POKE_FLUTE` (line 81)

Result: `EVENT_GOT_POKE_FLUTE` is the durable post-Tower progression flag.

### 2.3 Silph Co resolution (Saffron liberation chain)

Source: `pokered-master/scripts/SilphCo11F.asm` (+ references from Silph floors)

- Giovanni script gate:
  - `CheckEvent EVENT_BEAT_SILPH_CO_GIOVANNI` (line 167)
- Giovanni defeat completion:
  - `SetEvent EVENT_BEAT_SILPH_CO_GIOVANNI` (line 233)
- President gift branch:
  - `CheckEvent EVENT_GOT_MASTER_BALL` (line 295)
  - `SetEvent EVENT_GOT_MASTER_BALL` (line 304)

Cross-map consumers for Silph completion checks include:
- `SilphCo1F.asm` line 3
- `SilphCo3F.asm` line 76
- `SilphCo6F.asm` line 67
- `SilphCo7F.asm` lines 302, 346, 367, 388
- `SilphCo8F.asm` line 98
- `SilphCo9F.asm` line 146
- `SilphCo10F.asm` line 74

## 3) Gym 4–8 Completion Nodes (Post-Surge Region)

### 3.1 Koga (Gym 5)

Source: `pokered-master/scripts/FuchsiaGym.asm`

- `SetEvent EVENT_BEAT_KOGA` (line 52)
- Optional TM ownership: `SetEvent EVENT_GOT_TM06` (line 59)
- Re-talk checks:
  - `EVENT_BEAT_KOGA` (line 108)
  - `EVENT_GOT_TM06` (line 110)

### 3.2 Sabrina (Gym 6)

Source: `pokered-master/scripts/SaffronGym.asm`

- `SetEvent EVENT_BEAT_SABRINA` (line 50)
- Optional TM ownership: `SetEvent EVENT_GOT_TM46` (line 57)
- Re-talk checks:
  - `EVENT_BEAT_SABRINA` (line 109)
  - `EVENT_GOT_TM46` (line 111)

### 3.3 Blaine (Gym 7)

Source: `pokered-master/scripts/CinnabarGym.asm`

- `SetEvent EVENT_BEAT_BLAINE` (line 150)
- Optional TM ownership: `SetEvent EVENT_GOT_TM38` (line 157)
- Re-talk checks:
  - `EVENT_BEAT_BLAINE` (line 214)
  - `EVENT_GOT_TM38` (line 216)

### 3.4 Giovanni (Gym 8, Viridian)

Source: `pokered-master/scripts/ViridianGym.asm`

- `SetEvent EVENT_BEAT_VIRIDIAN_GYM_GIOVANNI` (line 142)
- Critical side effect for Rival 2 on Route 22:
  - `SetEvents EVENT_2ND_ROUTE22_RIVAL_BATTLE, EVENT_ROUTE22_RIVAL_WANTS_BATTLE` (line 167)
- Re-talk/gym guide checks consume `EVENT_BEAT_VIRIDIAN_GYM_GIOVANNI` (lines 209, 425)

## 4) Rival 2 Arm/Consume/Disarm Structure (Route 22, Late Game)

Arm source:
- `ViridianGym.asm` line 167 sets:
  - `EVENT_2ND_ROUTE22_RIVAL_BATTLE`
  - `EVENT_ROUTE22_RIVAL_WANTS_BATTLE`

Consume/disarm source: `pokered-master/scripts/Route22.asm`

- Trigger gate checks:
  - `CheckEvent EVENT_ROUTE22_RIVAL_WANTS_BATTLE` (line 59)
  - `CheckEventReuseA EVENT_2ND_ROUTE22_RIVAL_BATTLE` (line 74)
- Victory mark:
  - `SetEvent EVENT_BEAT_ROUTE22_RIVAL_2ND_BATTLE` (line 326)
- Cleanup disarm:
  - `ResetEvents EVENT_2ND_ROUTE22_RIVAL_BATTLE, EVENT_ROUTE22_RIVAL_WANTS_BATTLE` (line 375)
- Text state checks beat flag:
  - `CheckEvent EVENT_BEAT_ROUTE22_RIVAL_2ND_BATTLE` (line 401)

Historical note on initialization:
- Oak Lab explicitly resets the second-battle arm in early-game setup:
  - `OaksLab.asm` line 637 resets `EVENT_2ND_ROUTE22_RIVAL_BATTLE`
  - line 638 sets `EVENT_ROUTE22_RIVAL_WANTS_BATTLE` for the first encounter

## 5) Route 23 Badge Gate Model (Pre-Elite Entry)

Source: `pokered-master/scripts/Route23.asm`

Structure is dual-layer:
- Persistent per-guard pass events:
  - Event bit addressing starts at line 33 (`EVENT_PASSED_*BADGE_CHECK`)
  - Individual checks lines 155–191
- Live badge ownership validation:
  - `FlagActionPredef` test over `wObtainedBadges` inside `Route23CheckForBadgeScript`
  - success path sets the corresponding `EVENT_PASSED_*BADGE_CHECK` (line 219)

Important dependency implication:
- Passing each guard is not just badge possession; it also memoizes via event flags.

## 6) Elite Four to Champion Script Dependencies

### 6.1 Room-local trainer beat flags gate room geometry/progression

Sources:
- `LoreleisRoom.asm`: checks `EVENT_BEAT_LORELEIS_ROOM_TRAINER_0` (line 19)
- `BrunosRoom.asm`: checks `EVENT_BEAT_BRUNOS_ROOM_TRAINER_0` (line 17)
- `AgathasRoom.asm`: checks `EVENT_BEAT_AGATHAS_ROOM_TRAINER_0` (line 17)

These flags control whether exits are blocked/opened in each room’s map-load block logic.

### 6.2 Lance room lock and completion

Source: `pokered-master/scripts/LancesRoom.asm`

- Entrance lock state:
  - checked via `EVENT_LANCES_ROOM_LOCK_DOOR` (line 16)
  - armed one-time by `CheckAndSetEvent EVENT_LANCES_ROOM_LOCK_DOOR` (line 71)
- Lance completion:
  - `SetEvent EVENT_BEAT_LANCE` in Lance after-battle text script (line 156)
  - future room behavior checks `EVENT_BEAT_LANCE` (line 55)

### 6.3 Champion battle completion

Source: `pokered-master/scripts/ChampionsRoom.asm`

- Champion rival defeat sets:
  - `SetEvent EVENT_BEAT_CHAMPION_RIVAL` (line 98)
- Text/state branches thereafter check same event (line 253)

## 7) Hall of Fame Reset Boundary

Source: `pokered-master/scripts/HallOfFame.asm`

- Post-clear cleanup performs:
  - `ResetEventRange INDIGO_PLATEAU_EVENTS_START, INDIGO_PLATEAU_EVENTS_END, 1` (line 45)

Related lobby reset behavior also exists:
- `pokered-master/scripts/IndigoPlateauLobby.asm` line 14 resets:
  - `ResetEventRange INDIGO_PLATEAU_EVENTS_START, EVENT_LANCES_ROOM_LOCK_DOOR`

Dependency implication:
- Indigo progression is intentionally range-reset after clear; mismatched ranges/timing will cause repeat-lock or skipped-state issues.

## 8) Practical Diff Order (ASM Structure-First)

If we follow ASM dependencies strictly from this point:

1. Surge completion node integrity (`EVENT_BEAT_LT_SURGE`, badge bits).
2. Rocket Hideout B4F completion + key item flow (`EVENT_BEAT_ROCKET_HIDEOUT_GIOVANNI`, lift key behavior, Silph Scope pickup path).
3. Tower 6F/7F + Fuji + flute chain (`EVENT_BEAT_GHOST_MAROWAK` -> `EVENT_RESCUED_MR_FUJI` -> `EVENT_GOT_POKE_FLUTE`).
4. Silph Co 11F Giovanni completion (`EVENT_BEAT_SILPH_CO_GIOVANNI`) and city/object fallout.
5. Gym 5/6/7/8 leader completion + badge/TM branch events.
6. Viridian Gym side effect that arms Route 22 Rival 2.
7. Route 22 Rival 2 consume/disarm symmetry.
8. Route 23 badge-check dual layer (badge bits + passed-check events).
9. E4 room progression flags, Lance lock/beat flags, Champion beat flag.
10. Hall of Fame event-range reset parity.

This sequence matches the ASM’s own progression dependencies and minimizes false positives while diffing.
