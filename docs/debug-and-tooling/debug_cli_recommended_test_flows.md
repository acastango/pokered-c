# pokered Debug CLI — Recommended Test Flows

This playbook gives repeatable, high-signal test flows using the current Debug CLI stack.

Core references:

- [game_cli_commands.md](C:/Users/Anthony/pokered/docs/debug-and-tooling/game_cli_commands.md)
- [debug_cli_tools_guide.md](C:/Users/Anthony/pokered/docs/debug-and-tooling/debug_cli_tools_guide.md)

---

## 0) Baseline Harness (Use For Every Flow)

Use this sequence before any focused test so results are reproducible:

```text
quicksave pre_flow
battle_seed 42
rng_state
eventdiff snapshot
script_trace on
```

Run your scenario, then collect:

```text
eventdiff show
script_trace off
quickload pre_flow
```

Why:

- `battle_seed` + `rng_state` stabilizes RNG-dependent behavior.
- `eventdiff` captures progression side effects.
- `script_trace` catches script state transitions/regressions.

---

## 1) Checkpoint Safety / Drift Audit

Goal: verify a checkpoint does what we think, without mutating your live run.

```text
checkpoint verify koga_post
checkpoint verify giovanni_ready
checkpoint verify ss_anne_hm
```

Follow-up:

```text
checkpoint list
```

Pass criteria:

- Verify output shows expected map/position/badge/flag deltas.
- No persistent state change after each verify (it auto-restores).

---

## 2) Gym Leader + TM Retry Branches

Goal: validate pre-battle, post-battle-no-TM, and done-with-TM branches.

Example (Koga):

```text
gym_reset koga
story_guard koga
tmflow koga pre
tmflow koga post
tmflow koga done
```

Repeat for:

- `brock`, `misty`, `surge`, `erika`, `koga`, `blaine`

Pass criteria:

- `gym_reset <leader>` reliably teleports to correct leader setup tile.
- `tmflow post` shows post-win/no-TM branch behavior.
- `tmflow done` shows post-TM behavior.

---

## 3) Map-Wide Trainer Re-battle Sweep

Goal: quickly reset all trainers on a map and verify trainer scripts re-arm correctly.

```text
teleport fuchsia_gym
trainer_reset here
eventdiff snapshot
```

Battle one trainer, then:

```text
eventdiff show
trainer_reset fuchsia_gym
```

Also supported:

```text
trainer_reset 166
trainer_reset CeruleanGym
```

Pass criteria:

- All trainer defeat flags on target map clear.
- Trainers are interactable/engage again after reset.

---

## 4) Bike Gate Access Guard Flow

Goal: verify cycling-road gate eligibility behavior.

```text
story_guard bike_gate
teleport route_16_gate_1f
```

If bike missing:

```text
giveitem bicycle
story_guard bike_gate
```

Suggested instrumentation:

```text
script_trace on
eventdiff snapshot
```

Pass criteria:

- Without bike, guard/pushback behavior triggers.
- With bike, passage logic proceeds.

---

## 5) Rocket Hideout Repeatability Loop

Goal: hammer Lift Key + Giovanni sequences repeatedly without manual cleanup.

```text
checkpoint giovanni_ready
quicksave gio_loop
```

After each run:

```text
checkpoint giovanni_reset
checkpoint liftkey_reset
quickload gio_loop
```

Pass criteria:

- Giovanni and Lift Key logic returns to expected pre-fight/pre-pickup states each loop.

---

## 6) Teleport Spawn Fidelity (ASM Fly Destinations)

Goal: ensure town teleports land at stable Pokecenter-front coordinates.

```text
teleport pallet
teleport viridian
teleport pewter
teleport cerulean
teleport lavender
teleport vermilion
teleport celadon
teleport fuchsia
teleport cinnabar
teleport saffron
teleport indigo_plateau
```

Optional route fly spots:

```text
teleport route_4_fly
teleport route_10_fly
```

Pass criteria:

- Arrival tiles are valid/stable and match expected Fly destination behavior.

---

## 7) Story Progression Guardrails

Goal: preflight prerequisites before a scripted test to avoid false negatives.

```text
story_guard brock
story_guard misty
story_guard surge
story_guard erika
story_guard koga
story_guard blaine
```

Pass criteria:

- Missing prerequisites are explicit (flags/badges/items), not silent.

---

## 8) Script Transition Regression Capture

Goal: catch subtle map-script state machine regressions.

```text
script_trace on
teleport cerulean
checkpoint cerulean_rocket
wait 180
script_trace off
```

Artifacts:

- Console `[script_trace]` lines
- `bugs/script_trace.log`
- `bugs/script_trace.log.1` (rotated backup when size cap is hit)

Pass criteria:

- Expected script activations/transitions occur; no stuck active states.

---

## 9) Gold Master Replay Flow

Goal: preserve deterministic repros for hard bugs.

```text
battle_seed 42
replay record bike_gate_bug_01
```

Perform scenario, then:

```text
replay stop
replay play bike_gate_bug_01
```

Pair with:

```text
eventdiff snapshot
eventdiff show
```

Pass criteria:

- Replay reproduces the issue/behavior path.
- Side-effect deltas are comparable between runs.

---

## 10) Fast Smoke Suite (5-minute daily)

Run in order:

```text
checkpoint verify brock_post
checkpoint verify koga_post
gym_reset surge
tmflow surge post
trainer_reset here
story_guard bike_gate
script_trace on
teleport saffron
wait 60
script_trace off
```

What this catches quickly:

- checkpoint drift
- gym/TM branch regressions
- trainer reset regressions
- bike gate prerequisites
- script activity weirdness after map transitions

---

## Notes

- Prefer `checkpoint verify` before introducing new checkpoint presets.
- Keep one canonical seed per bug thread (`battle_seed`) for apples-to-apples diffs.
- If a flow fails, capture:
  - exact command sequence
  - `eventdiff show` output
  - `bugs/script_trace.log`
  - replay file name (if recorded)
