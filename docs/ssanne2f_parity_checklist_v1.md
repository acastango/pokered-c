# SSAnne2F Parity Checklist (ASM vs C)

Date: 2026-05-05  
Scope: `scripts/SSAnne2F.asm` vs C implementation in `src/game/ss_anne_scripts.c` + map data in `src/data/event_data.c`.

## Verdict

`SSAnne2F` is **mostly implemented in dedicated C logic** and aligns structurally with ASM flow, but has at least one **confirmed text translation mismatch**.

## ASM Behavior Surface

From [`pokered-master/scripts/SSAnne2F.asm`](/C:/Users/Anthony/pokered/pokered-master/scripts/SSAnne2F.asm):

- Script-pointer state machine:
  - default trigger
  - rival start battle
  - rival after battle
  - rival exit
  - noop
- Rival trigger coordinates:
  - `(36,8)` and `(37,8)`
- Rival approach movement:
  - down 3 or 4 tiles depending on player x
- Rival trainer selection:
  - `OPP_RIVAL2`, trainer no based on starter
- Battle text flow:
  - pre-battle text
  - defeated/victory text pointers
  - post-battle CUT-master text
- Rival exits and is hidden after sequence.

## C Implementation Evidence

From [`src/game/ss_anne_scripts.c`](/C:/Users/Anthony/pokered/src/game/ss_anne_scripts.c):

- Explicitly ports `scripts/SSAnne2F.asm` (file header comment).
- Has dedicated rival state machine (`RS_*`) with trigger, approach, battle pending, post-battle, walk-away, cleanup.
- Trigger matches ASM coords:
  - checks y=8 and x in `{36,37}`.
- Rival team selection by `wRivalStarter` mapped to trainer no `1/2/3`.
- Uses `EVENT_BEAT_SS_ANNE_RIVAL`, hides rival after completion.

From [`src/data/event_data.c`](/C:/Users/Anthony/pokered/src/data/event_data.c):

- `kWarps_SSAnne2F` and `kNpcs_SSAnne2F` present.
- Waiter and rival NPC entries present.
- Map binding:
  - `[0x60] = { kWarps_SSAnne2F, ..., kNpcs_SSAnne2F, ... }`

## Confirmed Differences / Gaps

## 1) Rival Post-Battle CUT-master Text Mismatch (Confirmed)

- ASM canonical text (`_SSAnne2FRivalCutMasterText`) in [`pokered-master/text/SSAnne2F.asm`](/C:/Users/Anthony/pokered/pokered-master/text/SSAnne2F.asm):
  - Starts with: `"<RIVAL>: I heard there was a CUT master on board."`
  - Includes lines about seasick old man and "You should go see him! Smell ya!"
- C current text in [`ss_anne_scripts.c`](/C:/Users/Anthony/pokered/src/game/ss_anne_scripts.c):
  - `kRivalPostBattle` uses different phrasing:
  - `"{RIVAL}: Hah! I won't lose next time! ... I'm going to see the CAPTAIN now! ... Later, {PLAYER}!"`
- Impact:
  - Medium (semantic translation fidelity issue, not structural flow break).

## 2) Structural Ownership Status Clarification

- This map should not be treated as "missing owner":
  - It has a dedicated owner module: [`ss_anne_scripts.c`](/C:/Users/Anthony/pokered/src/game/ss_anne_scripts.c).
- Prior matrix `partial` for `SSAnne2F.asm` is best interpreted as:
  - implemented via grouped module + custom runtime state machine,
  - not absent.

## What Appears Aligned

- Trigger coordinates and approach movement branching.
- Rival battle initiation path and trainer-no selection logic.
- Rival hide/cleanup sequencing.
- Core map/NPC data presence in `event_data.c`.

## Recommended Matrix Update Note

For `SSAnne2F.asm`:
- Keep ownership as present (`ss_anne_scripts.c` + `event_data.c`).
- Track one concrete fidelity issue:
  - post-battle CUT-master text content mismatch vs ASM text source.
