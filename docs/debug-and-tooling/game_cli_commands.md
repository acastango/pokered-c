# pokered game CLI — Command Reference

Run commands one-shot: `python tools/game_cli.py "<command>"`
Interactive mode: `python tools/game_cli.py`
Script file: `python tools/game_cli.py --script path/to/cmds.txt`

The game must be running. Commands are written to `bugs/cli_cmd.txt`;
results are read back from `bugs/cli_state.txt`.

Launch options:

- `pokered.exe --debug-render` → fixed `512x288` window (logical `256x144`, 2x scale) with CLI history sidebar

Runtime debug-render input notes:

- `~` / `Shift+~` toggles CLI typing mode
- Enter submits while typing mode is active
- Gameplay input is blocked while typing mode is active

---

## Input / Navigation

| Command | Description |
|---|---|
| `up [N]` | Walk N tiles north (default 1) |
| `down [N]` | Walk N tiles south |
| `left [N]` | Walk N tiles west |
| `right [N]` | Walk N tiles east |
| `a` / `interact` | Press A button |
| `b` / `back` | Press B button |
| `start` / `menu` | Press Start |
| `select` | Press Select |
| `wait [N]` | Idle N frames (default 60) |
| `state` | Refresh and print current state without acting |

---

## Teleport

```
teleport <map_id> [x y]     numeric map ID (decimal or 0x hex) + optional coords
teleport <name>             named location (see table below)
tele <...>                  alias for teleport
```

**Named locations:**

| Name | Location |
|---|---|
| `pallet` / `pallet_town` | Pallet Town |
| `viridian` / `viridian_city` | Viridian City |
| `pewter` / `pewter_city` | Pewter City |
| `cerulean` / `cerulean_city` | Cerulean City |
| `vermilion` / `vermilion_city` | Vermilion City |
| `lavender` / `lavender_town` | Lavender Town |
| `celadon` / `celadon_city` | Celadon City |
| `fuchsia` / `fuchsia_city` | Fuchsia City |
| `cinnabar` / `cinnabar_island` | Cinnabar Island |
| `indigo` / `indigo_plateau` | Indigo Plateau |
| `saffron` / `saffron_city` | Saffron City |
| `route_4_fly` | Route 4 Fly destination |
| `route_10_fly` | Route 10 Fly destination |
| `viridian_gym` | Viridian Gym |
| `pewter_gym` | Pewter Gym |
| `cerulean_gym` | Cerulean Gym |
| `vermilion_gym` | Vermilion Gym |
| `celadon_gym` | Celadon Gym |
| `fuchsia_gym` | Fuchsia Gym |
| `saffron_gym` | Saffron Gym |
| `cinnabar_gym` | Cinnabar Gym |
| `oaks_lab` | Oak's Lab |
| `viridian_forest` | Viridian Forest |
| `mt_moon` | Mt. Moon |
| `rock_tunnel` | Rock Tunnel |
| `pokemon_tower` | Pokemon Tower |
| `silph_co` | Silph Co. |
| `safari_zone` | Safari Zone |
| `route_1` … `route_12` | Routes 1-12 (south entrance) |
| `route_24`, `route_25` | Routes 24-25 |

**Examples:**
```
teleport cerulean
teleport 0x17 9 62        # Route 12, one tile west of Snorlax
```

---

## Items

| Command | Description |
|---|---|
| `giveitem <name\|id> [qty]` | Add item to bag. Name or hex/decimal ID. Default qty 1. |
| `givetm <n>` | Give TM n (1-50) |
| `givehm <n>` | Give HM n (1-5) |
| `listbag` | Print all items currently in bag |

**Named items for `giveitem`:**

```
poke_ball, great_ball, ultra_ball, master_ball
potion, super_potion, hyper_potion, max_potion, full_restore
antidote, burn_heal, ice_heal, awakening, parlyz_heal, full_heal
revive, max_revive
repel, super_repel, max_repel, escape_rope
fire_stone, thunder_stone, water_stone, moon_stone
rare_candy, poke_doll, nugget
town_map, bicycle, pokedex
poke_flute, pokeflute
oaks_parcel, parcel
ss_ticket
hm01, tm01
```

**Examples:**
```
giveitem pokeflute
giveitem potion 5
giveitem 0x49
givetm 28                 # TM28 = Dig
```

---

## Party

| Command | Description |
|---|---|
| `givemon <dex> [level]` | Add Pokemon to party by national dex number (1-151). Default level 20. Fills next slot or replaces slot 6 if full. |
| `bulba15` | One-shot setup: slot 1 becomes Bulbasaur Lv15 with Sleep Powder / Razor Leaf / Leech Seed / Vine Whip. Alias: `givemon_bulba15`. |
| `giveteam` | Replace party with Mewtwo/Dragonite/Alakazam/Machamp/Gengar/Lapras @ Lv100 |
| `teachmove <slot> <move_id>` | Teach move to party slot (1-based). Fills first empty move slot; overwrites slot 1 if full. Move ID decimal or hex. |
| `teach <slot> <move_id>` | Append move to party slot (1-based). Fills next empty slot; overwrites slot 4 if full. Sets PP to base PP. No restrictions. Move ID decimal or hex. |
| `sethealth <slot> <hp>` | Set current HP for party slot (1-based). Clamped to max HP. |
| `setlevel <slot> <level>` | Set level for party slot (1-based). Reinitialises stats and moves at new level. |
| `healparty` | Full HP + clear all status on every party mon |
| `addexp <slot> <amount>` | Directly add raw exp to a party slot. Handles level-up stat recalc and move learning silently. Does not trigger evolution. |
| `exprate <pct>` | Set battle exp multiplier as a percentage. 100=1×, 150=1.5×, 200=2×, 300=3×. Applied in every battle. Reset to 100 to restore normal. |

**Examples:**
```
givemon 1 15              # Bulbasaur Lv15
givemon 143 30            # Snorlax Lv30
givemon 6 50              # Charizard Lv50
teachmove 1 0x94          # Teach Flash to slot 1
teach 1 0x59              # Append Hyper Beam to slot 1 (pp=5)
sethealth 1 1             # Knock slot 1 to 1 HP
setlevel 2 50
healparty
addexp 1 5000             # Add 5000 exp to slot 1
exprate 300               # 3× exp from all battles
exprate 100               # reset to normal
```

---

## Money

| Command | Description |
|---|---|
| `setmoney <amount>` | Set player money 0-999999 |

---

## Flags & Badges

| Command | Description |
|---|---|
| `setflag <n>` | Set event flag n (decimal or 0x hex) |
| `clearflag <n>` | Clear event flag n |
| `givebadge <n>` | Set badge bit n (0=Boulder, 1=Cascade, 2=Thunder, 3=Rainbow, 4=Soul, 5=Marsh, 6=Volcano, 7=Earth) |

**Examples:**
```
givebadge 0               # give Boulder Badge
setflag 1167              # EVENT_BEAT_ROUTE12_SNORLAX
clearflag 0x48E           # clear a flag by hex
```

---

## Story Checkpoints

Jump to a pre-configured story state (sets relevant event flags, warps, gives party if empty):

| Command | State |
|---|---|
| `checkpoint parcel_ready` | Has starter + battled rival. Warps to Viridian Mart to pick up Oak's Parcel. |
| `checkpoint pokedex_ready` | Has parcel. Warps to Oak's Lab entrance ready to deliver. |
| `checkpoint mt_moon` | Through Brock. Warps to Mt. Moon B2F near fossil room. |
| `checkpoint cerulean` | Through Mt. Moon. Warps to Cerulean City bridge (rival trigger nearby). |
| `checkpoint route22` | Rival trigger setup on Route 22. |
| `checkpoint brock` | Inside Pewter Gym, Brock unbeaten. |
| `checkpoint misty` | Inside Cerulean Gym, Misty unbeaten. |
| `checkpoint surge` | Inside Vermilion Gym, Lt. Surge unbeaten. |
| `checkpoint erika` | Inside Celadon Gym, Erika unbeaten. |
| `checkpoint koga` | Inside Fuchsia Gym, Koga unbeaten. |
| `checkpoint blaine` | Inside Cinnabar Gym, Blaine unbeaten. |
| `checkpoint brock_post` | Brock beaten, TM34 not obtained. |
| `checkpoint misty_post` | Misty beaten, TM11 not obtained. |
| `checkpoint erika_post` | Erika beaten, TM21 not obtained. |
| `checkpoint koga_post` | Koga beaten, TM06 not obtained. |
| `checkpoint blaine_post` | Blaine beaten, TM38 not obtained. |
| `checkpoint gym_badges1..5` | Utility badge/event bundles for early-to-mid gym progression. |
| `checkpoint cerulean_rocket` | Cerulean Rocket thief trigger setup. |
| `checkpoint ss_anne_hm` | SS Anne HM/departure setup. |
| `checkpoint liftkey_reset` | Reset Lift Key drop/pickup state in Rocket Hideout B4F. |
| `checkpoint giovanni_reset` | Reset Giovanni + Silph Scope pickup state in Rocket Hideout B4F. |
| `checkpoint giovanni_ready` | Warp in front of Giovanni with prerequisites set. |
| `checkpoint list` / `checkpoint help` | List available checkpoints. |
| `checkpoint verify <name>` | Dry-run: show checkpoint side effects, then restore original state. |

---

## Debug / Diagnostics

| Command | Description |
|---|---|
| `tileinfo` / `tile_info` | Dump tile IDs and passability for player position + 4 neighbours |
| `eventdiff snapshot` | Capture current state snapshot for later diff |
| `eventdiff show` | Show map/pos/badge/key-flag changes since snapshot |
| `battle_seed <0-255>` | Set deterministic RNG seed bytes (`hRandomAdd`, `hRandomSub`) |
| `rng_state` | Print current RNG bytes + frame counter |
| `script_trace on|off|status` | Toggle script-state transition tracing |
| `script_trace file_on|file_off` | Toggle trace file logging |
| `story_guard <name>` | Assert prerequisite flags/badges/items for key scenarios |
| `replay record <name>` | Start deterministic replay capture (snapshot + per-frame inputs) |
| `replay stop` | Stop recording or stop playback |
| `replay play <name>` | Load replay snapshot and play recorded input stream |
| `replay status` | Show replay state and playback position |
| `scene_run <name>` | Run scene file `bugs/scenes/<name>.scene` |
| `scene_stop` | Stop active scene and unlock input |
| `scene_trigger set <scene> trigger_point <x_expr> <y_expr> [map]` | Register tile trigger that auto-runs a scene |
| `scene_trigger list` | List configured scene trigger points |
| `scene_trigger clear [scene]` | Clear all scene triggers, or only one scene name |

`story_guard` targets:
- `brock`, `misty`, `surge`, `erika`, `koga`, `blaine`, `bike_gate`, `list`

Script trace file outputs:
- `bugs/script_trace.log`
- rotated backup: `bugs/script_trace.log.1`

Scene trigger examples:

```txt
scene_trigger set duo trigger_point player.x-1 player.y
scene_trigger set npc_walkoff trigger_point player.x player.y-1 pallet_town
scene_trigger list
scene_trigger clear duo
scene_trigger clear
```

Coordinate expressions accepted by `scene_trigger set`:
- absolute numeric tile coordinate: `12`, `5`
- player-relative x: `player.x`, `player.x+2`, `player.x-1`
- player-relative y: `player.y`, `player.y+1`, `player.y-3`

---

## Trainer / Gym Reset Helpers

| Command | Description |
|---|---|
| `trainer_reset <event_flag> clear|set` | Legacy single-flag trainer defeat toggle |
| `trainer_reset here` | Clear all trainer-defeat flags on current map |
| `trainer_reset <map_id>` | Clear all trainer-defeat flags on map by numeric ID |
| `trainer_reset <map_name>` | Clear all trainer-defeat flags on map by name |
| `gym_reset <leader>` | Reset one gym (leader/trainers/TM) and auto-teleport to leader |
| `gym_reset all` | Reset Brock..Blaine gym progress bundles |
| `tmflow <leader> <pre|post|done|reset>` | Standardize post-leader TM branch testing |
| `gym_badges_clear <n>` | Keep badges 1..n; clear later gym leader/trainer/TM progress |

Rewind hotkeys (runtime, not CLI commands):

- Hold `Ctrl+Z` → frame-by-frame rewind
- Hold `Ctrl+Y` → frame-by-frame forward (redo)

Session logs (runtime outputs):

- `bugs/session_narrative.log` (human-readable)
- `bugs/session_events.jsonl` (structured events, including event-flag transitions and battle start/end summaries)

---

## Battle Commands

These navigate the battle menu via button sequences (use while in battle):

| Command | Description |
|---|---|
| `fight [1-4]` | Open FIGHT menu and select move slot 1-4 |
| `run` | Select RUN from battle menu |
| `pkmn` / `pokemon` | Open PKMN menu in battle |
| `bag` / `item` | Open BAG/ITEM menu in battle |
