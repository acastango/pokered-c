# pokered game CLI — Command Reference

Run commands one-shot: `python tools/game_cli.py "<command>"`
Interactive mode: `python tools/game_cli.py`
Script file: `python tools/game_cli.py --script path/to/cmds.txt`

The game must be running. Commands are written to `bugs/cli_cmd.txt`;
results are read back from `bugs/cli_state.txt`.

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
| `saffron` / `saffron_city` | Saffron City |
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

---

## Debug / Diagnostics

| Command | Description |
|---|---|
| `tileinfo` / `tile_info` | Dump tile IDs and passability for player position + 4 neighbours |

---

## Battle Commands

These navigate the battle menu via button sequences (use while in battle):

| Command | Description |
|---|---|
| `fight [1-4]` | Open FIGHT menu and select move slot 1-4 |
| `run` | Select RUN from battle menu |
| `pkmn` / `pokemon` | Open PKMN menu in battle |
| `bag` / `item` | Open BAG/ITEM menu in battle |
