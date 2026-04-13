# Changelog

## 2026-04-13

### Highlights
- Added major script/gameplay coverage for the S.S. Anne line, including rival battle flow, Captain HM01 Cut handoff, and expanded trainer/event support.
- Shipped large ASM-fidelity sync across battle UI, Pokédex, Bills House, event scripting, and PC systems.
- Added comprehensive debug and data plumbing needed for playtest-driven iteration.

### Added
- Implemented Route 22 rival script path and integrated it into world scripting.
- Added full PC menu scaffolding and commands (`Someone's PC`, player/Oak branches, storage operations, box controls).
- Added YES/NO helper system and integrated it into missing classic interaction loops.
- Added TM/HM runtime/data pipeline (`tmhm.c`, generated `tmhm_data.*`) and supporting tool scripts.
- Added Poké Flute and field-move support modules.
- Added generated event-flag name table for human-readable debug output.
- Added Pokédex tile/data assets for closer visual/flow parity.
- Added storage regression tests (`tests/test_pokemon_storage.c`).

### Changed
- Expanded S.S. Anne scripts and map event wiring for stronger parity with original sequencing.
- Reworked Pokédex list/detail behavior to follow original-style flow:
  - list selection action menu (`DATA/CRY/AREA/QUIT`)
  - improved data-page load sequencing and interaction behavior
  - improved visual framing and tile usage
- Updated battle text/state sequencing and transition timing in `battle_ui` to reduce non-ASM behavior drift.
- Updated audio scheduling/handling paths for improved SFX/music behavior under heavy event transitions.
- Updated data generators and extracted tables (moves/dex/music/events) to reflect current engine state.

### Fixed
- Tail Whip miss regression: corrected stat-down miss path so non-link 25% miss quirk is applied on the correct turn side.
- Bubble behavior path updated so damage/effect behavior no longer routes incorrectly.
- Wild-capture Pokédex ownership tracking fixed for caught Pokémon.
- Bill’s House initial encounter fixed to restore missing YES/NO loop behavior.
- Bill’s post-help PC behavior restored toward original expected flow.
- Nugget Bridge reward/dialog jingle timing fixed.
- Multiple rival-battle integration issues addressed (including Route 22/Cerulean interplay).
- Battle switch/capture/evolution presentation issues reduced via UI and sequence fixes.
- PC storage integrity fixes:
  - preserve Pokémon data on deposit/withdraw
  - avoid corrupted level/state on retrieval
  - improved withdraw text/message parity
- Fixed multiple UI/layout regressions in menu/textbox overlays and tile alignment.

### Tooling / Debug
- Expanded `debug_cli` command surface for faster scenario setup and verification.
- Improved event-flag observability with named flag output.
- Added/updated helper docs for CLI commands and generator scripts.

### Notes
- This release includes broad ASM-fidelity work in progress and supporting infrastructure; some systems are still under active parity refinement.
- Detailed file-by-file inventory for this release is tracked in:
  - `bugs/release_ledger_2026-04-13.md`

