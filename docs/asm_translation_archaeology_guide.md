# ASM Translation Archaeology Guide (Post–Lt. Surge)

Purpose:
- Turn story/event parity work into granular, verifiable translation units.
- Preserve original ASM structure (script labels, state transitions, side effects).
- Make implementation deterministic: one row -> one concrete parity check.

Primary dataset:
- `docs/asm_translation_archaeology_rows_post_lt_surge.csv`

## How to use this dataset

1. Work in `dependency_id` order unless blocked.
2. For each row:
   - Verify ASM behavior at `asm_anchor`.
   - Locate current C ownership via `c_owner_candidates`.
   - Confirm or fill `c_function_candidates`.
   - Implement parity for exactly this unit.
   - Run the listed `probe`.
   - Mark `status` + `verified_owner` + `verification_notes`.
3. Never skip side effects:
   - badge bits, toggle objects, script-state writes, map-load flags, and range resets.

## Status meanings

- `missing`: no equivalent behavior found in current C path.
- `partial`: behavior exists but differs in trigger/branch/cleanup semantics.
- `implemented`: behavior appears parity-correct and verified.
- `needs_test`: code seems present but not yet runtime-verified.

## Row anatomy

- `dependency_id`: topological ordering key.
- `unit_id`: stable reference for commits/notes.
- `unit_type`: `event_set`, `event_check`, `event_reset`, `state_transition`, `badge_bit`, `toggle_object`, `range_reset`, `movement_gate`, `item_branch`.
- `asm_anchor`: `scripts/<file>.asm:<line>` exact source of truth.
- `preconditions`/`postconditions`: behavioral contract.
- `side_effects`: required non-event outputs (music, objects, map flags, joy ignore, etc.).
- `c_owner_candidates`: likely owner files to inspect first.
- `c_function_candidates`: expected function-level ownership.
- `probe`: quickest runtime check to prove parity.

## Suggested implementation cadence

1. `SURGE_*` + `RH_*` + `PT_*` rows (post-gym3 story unlock).
2. `SILPH11_*` rows (city liberation branch effects).
3. `GYM5_*` to `GYM8_*` rows.
4. `R22B_*` rows (Rival 2 arm/consume/disarm).
5. `R23_*` rows (badge gate model).
6. `E4_*`/`LANCE_*`/`CHAMP_*`/`HOF_*`.

## Commit hygiene recommendation

- One commit per small cluster (3-8 rows) with message suffix listing `unit_id`s.
- Example: `parity(route22): arm rival2 + consume/disarm symmetry [R22B_ARM,R22B_SET_BEAT,R22B_DISARM]`
