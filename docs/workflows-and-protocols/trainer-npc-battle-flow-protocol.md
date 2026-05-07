# Trainer NPC Battle Flow Protocol (Reusable)

Use this whenever adding trainer NPCs on routes/caves/gyms/dojo.
This captures the current canonical C implementation in this repo.

## Scope

- Route/cave style trainers (Viridian Forest, Mt. Moon, etc.)
- Gym/dojo trainers that use trainer flags and pre/post-battle text
- Includes both sight-trigger and A-button interact behavior

## Source of truth in current code

- Trainer record schema: `src/data/event_data.h` (`map_trainer_t`)
- A-button trainer dispatch: `src/game/game.c` (`check_npc_interact`)
- Sight detection / walk-up / pre-battle text: `src/game/trainer_sight.c`
- Spawn facing initialization: `src/game/npc.c` (`NPC_Load` + trainer facing override)
- Map examples:
  - `kTrainers_ViridianForest`
  - `kTrainers_MtMoon1F`
  - `kTrainers_MtMoonB2F`
  - `kTrainers_PewterGym`
  - `kTrainers_CeruleanGym`
  - `kTrainers_CeladonGym`
  - `kTrainers_FightingDojo`

## Data model you must wire

Each trainer entry is:

`{ npc_idx, facing, trainer_class, trainer_no, sight_dist, flag_bit, before_text, after_text, end_text }`

- `npc_idx`: index in that map's `kNpcs_*` array
- `facing`: `0=down, 1=up, 2=left, 3=right`
- `trainer_class`: raw class id (not `OPP_ID_OFFSET` form)
- `trainer_no`: 1-based party index in that class
- `sight_dist`: engage distance in map units
- `flag_bit`: event bit set after trainer defeat
- `before_text`: shown before battle
- `after_text`: shown on later interactions after defeat
- `end_text`:
  - `NULL` for standard route/cave trainers
  - non-`NULL` for gym-script routed trainers where victory flow is custom

## Canonical wiring steps

1. Add/confirm NPC slot in `kNpcs_MapName[]`.
- Trainer NPC can keep `text=NULL, script=NULL` for standard route/cave flow.
- For gym leaders or custom maps, NPC script callbacks can still be used; trainer entries still drive battle metadata.

2. Add `kTrainers_MapName[]`.
- One entry per trainer NPC.
- Match `npc_idx` exactly to `kNpcs_MapName[]` position.
- Set `facing` to ASM-facing. This now also controls initial spawn facing via `NPC_Load`.

3. Attach trainer table in `gMapEvents[...]`.
- Example shape:
`[map_id] = { ..., kTrainers_MapName, trainer_count }`

4. Ensure trainer flags exist in `event_constants.h`.
- One unique `EVENT_BEAT_*` per trainer.
- Reuse existing event constants where already defined by extraction.

5. For gyms with custom post-win flow:
- Set `end_text` in trainer entries.
- Keep gym leader/NPC callbacks routed through `gym_scripts.c` as done in existing gyms.

## Runtime behavior (what this wiring gives you)

### A-button interaction path (`game.c`)

- If trainer is undefeated (`!CheckEvent(flag_bit)`): immediate engage via `Trainer_EngageImmediate`.
- If defeated: show `after_text`.

### Sight path (`trainer_sight.c`)

- Checks line-of-sight by axis + `sight_dist` + facing direction.
- Shows `!` emote and encounter music.
- Trainer walks toward player until adjacent.
- Shows `before_text`.
- Signals battle start.

### Post-battle

- On defeat, the trainer's event flag is set.
- Future interaction uses `after_text`.
- Trainer remains present (unless a map-specific script hides/replaces them).

## Minimal template (copy/paste)

```c
static const npc_event_t kNpcs_ExampleMap[] = {
    { 10, 12, 0x04, 0, NULL, NULL },  /* trainer npc idx 0 */
};

static const map_trainer_t kTrainers_ExampleMap[] = {
    { 0, 3, 1, 4, 3, EVENT_BEAT_EXAMPLE_TRAINER_0,
      "Before battle text",
      "After battle text",
      NULL },
};

/* in gMapEvents[...] */
[0xNN] = { ..., kNpcs_ExampleMap, 1, ..., kTrainers_ExampleMap, 1 },
```

## Common mistakes to avoid

- Missing `kTrainers_*` attach in `gMapEvents`: NPC exists but never battles.
- Wrong `npc_idx`: wrong NPC battles or no battle.
- Wrong `trainer_class`/`trainer_no`: incorrect roster.
- Wrong facing: bad LOS triggers and wrong spawn orientation.
- Reusing a flag across multiple trainers unintentionally.
- Using `OPP_*` constants directly in `trainer_class` (table expects raw class id).

## Verification checklist (quick)

- Approach into LOS: trainer spots, emote plays, walks, pre-text, battle starts.
- Talk with A before battle: battle still starts correctly.
- After win: re-talk shows `after_text`, no rematch.
- Save/load: defeated trainer stays defeated.
- Facing on map load matches ASM.
