#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_data.py -- Extract pokered ASM data tables into C source files.

Generates:
  src/data/base_stats.h / .c
  src/data/moves_data.h / .c
  src/data/type_chart.h / .c
"""

import os, re, sys

SRC = r"C:\Users\Anthony\pokered\pokered-master"
OUT = r"C:\Users\Anthony\pokered\src\data"

os.makedirs(OUT, exist_ok=True)

def wfile(name, content):
    path = os.path.join(OUT, name)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"  {path}  ({len(content)} chars)")

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

TYPE_IDS = {
    "NORMAL": 0x00, "FIGHTING": 0x01, "FLYING": 0x02,
    "POISON": 0x03, "GROUND": 0x04, "ROCK": 0x05,
    "BIRD": 0x06, "BUG": 0x07, "GHOST": 0x08,
    "FIRE": 0x14, "WATER": 0x15, "GRASS": 0x16,
    "ELECTRIC": 0x17, "PSYCHIC_TYPE": 0x18, "ICE": 0x19,
    "DRAGON": 0x1A,
}

GROWTH_RATES = {
    "GROWTH_MEDIUM_FAST": 0, "GROWTH_SLIGHTLY_FAST": 1,
    "GROWTH_SLIGHTLY_SLOW": 2, "GROWTH_MEDIUM_SLOW": 3,
    "GROWTH_FAST": 4, "GROWTH_SLOW": 5,
}

MOVE_IDS = {
    "NO_MOVE": 0,
    "POUND": 1, "KARATE_CHOP": 2, "DOUBLESLAP": 3, "COMET_PUNCH": 4,
    "MEGA_PUNCH": 5, "PAY_DAY": 6, "FIRE_PUNCH": 7, "ICE_PUNCH": 8,
    "THUNDERPUNCH": 9, "SCRATCH": 10, "VICEGRIP": 11, "GUILLOTINE": 12,
    "RAZOR_WIND": 13, "SWORDS_DANCE": 14, "CUT": 15, "GUST": 16,
    "WING_ATTACK": 17, "WHIRLWIND": 18, "FLY": 19, "BIND": 20,
    "SLAM": 21, "VINE_WHIP": 22, "STOMP": 23, "DOUBLE_KICK": 24,
    "MEGA_KICK": 25, "JUMP_KICK": 26, "ROLLING_KICK": 27, "SAND_ATTACK": 28,
    "HEADBUTT": 29, "HORN_ATTACK": 30, "FURY_ATTACK": 31, "HORN_DRILL": 32,
    "TACKLE": 33, "BODY_SLAM": 34, "WRAP": 35, "TAKE_DOWN": 36,
    "THRASH": 37, "DOUBLE_EDGE": 38, "TAIL_WHIP": 39, "POISON_STING": 40,
    "TWINEEDLE": 41, "PIN_MISSILE": 42, "LEER": 43, "BITE": 44,
    "GROWL": 45, "ROAR": 46, "SING": 47, "SUPERSONIC": 48,
    "SONICBOOM": 49, "DISABLE": 50, "ACID": 51, "EMBER": 52,
    "FLAMETHROWER": 53, "MIST": 54, "WATER_GUN": 55, "HYDRO_PUMP": 56,
    "SURF": 57, "ICE_BEAM": 58, "BLIZZARD": 59, "PSYBEAM": 60,
    "BUBBLEBEAM": 61, "AURORA_BEAM": 62, "HYPER_BEAM": 63, "PECK": 64,
    "DRILL_PECK": 65, "SUBMISSION": 66, "LOW_KICK": 67, "COUNTER": 68,
    "SEISMIC_TOSS": 69, "STRENGTH": 70, "ABSORB": 71, "MEGA_DRAIN": 72,
    "LEECH_SEED": 73, "GROWTH": 74, "RAZOR_LEAF": 75, "SOLARBEAM": 76,
    "POISONPOWDER": 77, "STUN_SPORE": 78, "SLEEP_POWDER": 79,
    "PETAL_DANCE": 80, "STRING_SHOT": 81, "DRAGON_RAGE": 82,
    "FIRE_SPIN": 83, "THUNDERSHOCK": 84, "THUNDERBOLT": 85,
    "THUNDER_WAVE": 86, "THUNDER": 87, "ROCK_THROW": 88, "EARTHQUAKE": 89,
    "FISSURE": 90, "DIG": 91, "TOXIC": 92, "CONFUSION": 93,
    "PSYCHIC_M": 94, "HYPNOSIS": 95, "MEDITATE": 96, "AGILITY": 97,
    "QUICK_ATTACK": 98, "RAGE": 99, "TELEPORT": 100, "NIGHT_SHADE": 101,
    "MIMIC": 102, "SCREECH": 103, "DOUBLE_TEAM": 104, "RECOVER": 105,
    "HARDEN": 106, "MINIMIZE": 107, "SMOKESCREEN": 108, "CONFUSE_RAY": 109,
    "WITHDRAW": 110, "DEFENSE_CURL": 111, "BARRIER": 112, "LIGHT_SCREEN": 113,
    "HAZE": 114, "REFLECT": 115, "FOCUS_ENERGY": 116, "BIDE": 117,
    "METRONOME": 118, "MIRROR_MOVE": 119, "SELFDESTRUCT": 120, "EGG_BOMB": 121,
    "LICK": 122, "SMOG": 123, "SLUDGE": 124, "BONE_CLUB": 125,
    "FIRE_BLAST": 126, "WATERFALL": 127, "CLAMP": 128, "SWIFT": 129,
    "SKULL_BASH": 130, "SPIKE_CANNON": 131, "CONSTRICT": 132, "AMNESIA": 133,
    "KINESIS": 134, "SOFTBOILED": 135, "HI_JUMP_KICK": 136, "GLARE": 137,
    "DREAM_EATER": 138, "POISON_GAS": 139, "BARRAGE": 140, "LEECH_LIFE": 141,
    "LOVELY_KISS": 142, "SKY_ATTACK": 143, "TRANSFORM": 144, "BUBBLE": 145,
    "DIZZY_PUNCH": 146, "SPORE": 147, "FLASH": 148, "PSYWAVE": 149,
    "SPLASH": 150, "ACID_ARMOR": 151, "CRABHAMMER": 152, "EXPLOSION": 153,
    "FURY_SWIPES": 154, "BONEMERANG": 155, "REST": 156, "ROCK_SLIDE": 157,
    "HYPER_FANG": 158, "SHARPEN": 159, "CONVERSION": 160, "TRI_ATTACK": 161,
    "SUPER_FANG": 162, "SLASH": 163, "SUBSTITUTE": 164, "STRUGGLE": 165,
}

# Species internal ID -> Pokedex number
SPECIES_TO_DEX = {
    0x99: 1,   0x09: 2,   0x9A: 3,   0xB0: 4,   0xB2: 5,
    0xB4: 6,   0xB1: 7,   0xB3: 8,   0x1C: 9,   0x7B: 10,
    0x7C: 11,  0x7D: 12,  0x70: 13,  0x71: 14,  0x72: 15,
    0x24: 16,  0x96: 17,  0x97: 18,  0xA5: 19,  0xA6: 20,
    0x05: 21,  0x23: 22,  0x6C: 23,  0x2D: 24,  0x54: 25,
    0x55: 26,  0x60: 27,  0x61: 28,  0x0F: 29,  0xA8: 30,
    0x10: 31,  0x03: 32,  0xA7: 33,  0x07: 34,  0x04: 35,
    0x8E: 36,  0x52: 37,  0x53: 38,  0x64: 39,  0x65: 40,
    0x6B: 41,  0x82: 42,  0xB9: 43,  0xBA: 44,  0xBB: 45,
    0x6D: 46,  0x2E: 47,  0x41: 48,  0x77: 49,  0x3B: 50,
    0x76: 51,  0x4D: 52,  0x90: 53,  0x2F: 54,  0x80: 55,
    0x39: 56,  0x75: 57,  0x21: 58,  0x14: 59,  0x47: 60,
    0x6E: 61,  0x6F: 62,  0x94: 63,  0x26: 64,  0x95: 65,
    0x6A: 66,  0x29: 67,  0x7E: 68,  0xBC: 69,  0xBD: 70,
    0xBE: 71,  0x18: 72,  0x9B: 73,  0xA9: 74,  0x27: 75,
    0x31: 76,  0xA3: 77,  0xA4: 78,  0x25: 79,  0x08: 80,
    0xAD: 81,  0x36: 82,  0x40: 83,  0x46: 84,  0x74: 85,
    0x3A: 86,  0x78: 87,  0x0D: 88,  0x88: 89,  0x17: 90,
    0x8B: 91,  0x19: 92,  0x93: 93,  0x0E: 94,  0x22: 95,
    0x30: 96,  0x81: 97,  0x4E: 98,  0x8A: 99,  0x06: 100,
    0x8D: 101, 0x0C: 102, 0x0A: 103, 0x11: 104, 0x91: 105,
    0x2B: 106, 0x2C: 107, 0x0B: 108, 0x37: 109, 0x8F: 110,
    0x12: 111, 0x01: 112, 0x28: 113, 0x1E: 114, 0x02: 115,
    0x5C: 116, 0x5D: 117, 0x9D: 118, 0x9E: 119, 0x1B: 120,
    0x98: 121, 0x2A: 122, 0x1A: 123, 0x48: 124, 0x35: 125,
    0x33: 126, 0x1D: 127, 0x3C: 128, 0x85: 129, 0x16: 130,
    0x13: 131, 0x4C: 132, 0x66: 133, 0x69: 134, 0x68: 135,
    0x67: 136, 0xAA: 137, 0x62: 138, 0x63: 139, 0x5A: 140,
    0x5B: 141, 0xAB: 142, 0x84: 143, 0x4A: 144, 0x4B: 145,
    0x49: 146, 0x58: 147, 0x59: 148, 0x42: 149, 0x83: 150,
    0x15: 151,
}

# ---------------------------------------------------------------------------
# Base stats parsing
# ---------------------------------------------------------------------------

BASE_STATS_ORDER = [
    "bulbasaur","ivysaur","venusaur","charmander","charmeleon","charizard",
    "squirtle","wartortle","blastoise","caterpie","metapod","butterfree",
    "weedle","kakuna","beedrill","pidgey","pidgeotto","pidgeot",
    "rattata","raticate","spearow","fearow","ekans","arbok",
    "pikachu","raichu","sandshrew","sandslash","nidoranf","nidorina",
    "nidoqueen","nidoranm","nidorino","nidoking","clefairy","clefable",
    "vulpix","ninetales","jigglypuff","wigglytuff","zubat","golbat",
    "oddish","gloom","vileplume","paras","parasect","venonat","venomoth",
    "diglett","dugtrio","meowth","persian","psyduck","golduck",
    "mankey","primeape","growlithe","arcanine","poliwag","poliwhirl",
    "poliwrath","abra","kadabra","alakazam","machop","machoke","machamp",
    "bellsprout","weepinbell","victreebel","tentacool","tentacruel",
    "geodude","graveler","golem","ponyta","rapidash","slowpoke","slowbro",
    "magnemite","magneton","farfetchd","doduo","dodrio","seel","dewgong",
    "grimer","muk","shellder","cloyster","gastly","haunter","gengar",
    "onix","drowzee","hypno","krabby","kingler","voltorb","electrode",
    "exeggcute","exeggutor","cubone","marowak","hitmonlee","hitmonchan",
    "lickitung","koffing","weezing","rhyhorn","rhydon","chansey","tangela",
    "kangaskhan","horsea","seadra","goldeen","seaking","staryu","starmie",
    "mrmime","scyther","jynx","electabuzz","magmar","pinsir","tauros",
    "magikarp","gyarados","lapras","ditto","eevee","vaporeon","jolteon",
    "flareon","porygon","omanyte","omastar","kabuto","kabutops","aerodactyl",
    "snorlax","articuno","zapdos","moltres","dratini","dragonair","dragonite",
    "mewtwo",
]

def parse_stats(path):
    text = open(path, encoding="utf-8", errors="replace").read()
    text = re.sub(r";[^\n]*", "", text)  # strip comments
    s = {}

    # hp atk def spd spc
    m = re.search(r"db\s+(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)", text)
    if m:
        s["hp"], s["atk"], s["def"], s["spd"], s["spc"] = (int(m.group(i)) for i in range(1, 6))

    # type1, type2 (first line that matches two type words)
    m = re.search(r"db\s+([A-Z_]+)\s*,\s*([A-Z_]+)", text)
    if m and m.group(1) in TYPE_IDS:
        s["type1"] = TYPE_IDS.get(m.group(1), 0)
        s["type2"] = TYPE_IDS.get(m.group(2), 0)

    # catch rate and base exp: two single-number db lines after types
    single_nums = re.findall(r"^\s*db\s+(\d+)\s*$", text, re.MULTILINE)
    if len(single_nums) >= 2:
        s["catch_rate"] = int(single_nums[0])
        s["base_exp"]   = int(single_nums[1])

    # growth rate (find it first, then look for moves line just before it)
    m = re.search(r"GROWTH_\w+", text)
    s["growth_rate"] = GROWTH_RATES.get(m.group(0) if m else "", 0)

    # starting moves: the db line immediately before the GROWTH_ line
    if m:
        before_growth = text[:m.start()]
        # Last db line with alpha identifiers before GROWTH_
        mm = None
        for mm in re.finditer(r"db\s+([A-Z_]+(?:\s*,\s*[A-Z_]+){0,3})", before_growth):
            pass  # iterate to last match
        if mm:
            raw = [x.strip() for x in mm.group(1).split(",")]
            s["moves"] = [MOVE_IDS.get(mv, 0) for mv in raw[:4]]
            while len(s["moves"]) < 4:
                s["moves"].append(0)
    if "moves" not in s:
        s["moves"] = [0, 0, 0, 0]

    return s

base_stats_dir = os.path.join(SRC, "data", "pokemon", "base_stats")
all_stats = []
for name in BASE_STATS_ORDER:
    fpath = os.path.join(base_stats_dir, f"{name}.asm")
    if not os.path.exists(fpath):
        print(f"WARNING: missing {fpath}", file=sys.stderr)
        all_stats.append({"name": name})
    else:
        s = parse_stats(fpath)
        s["name"] = name
        all_stats.append(s)

print(f"Parsed {len(all_stats)} Pokemon base stats")

# ---------------------------------------------------------------------------
# base_stats.h
# ---------------------------------------------------------------------------

wfile("base_stats.h", """\
#pragma once
/* base_stats.h -- Generated from pokered-master/data/pokemon/base_stats/<all>.asm
 *
 * gBaseStats is indexed by POKEDEX NUMBER (1-based, 0 unused).
 * gSpeciesToDex converts internal species ID to Pokedex number (1..151).
 */
#include <stdint.h>
#include "game/types.h"

extern const base_stats_t gBaseStats[152];
extern const uint8_t gSpeciesToDex[256];
""")

# ---------------------------------------------------------------------------
# base_stats.c
# ---------------------------------------------------------------------------

lines = [
    "/* base_stats.c -- Generated from pokered-master/data/pokemon/base_stats/<all>.asm */",
    '#include "base_stats.h"',
    "",
    "const base_stats_t gBaseStats[152] = {",
    "    [0] = {0}, /* unused */",
]

for i, s in enumerate(all_stats):
    dex = i + 1
    if not s.get("hp"):
        lines.append(f"    [{dex}] = {{0}}, /* {s['name']} -- data missing */")
        continue
    mv = s.get("moves", [0,0,0,0])
    lines.append(
        f"    [{dex}] = {{ "
        f".dex_id={dex}, "
        f".hp={s['hp']}, .atk={s['atk']}, .def={s['def']}, "
        f".spd={s['spd']}, .spc={s['spc']}, "
        f".type1=0x{s.get('type1',0):02X}, .type2=0x{s.get('type2',0):02X}, "
        f".catch_rate={s.get('catch_rate',0)}, .base_exp={s.get('base_exp',0)}, "
        f".start_moves={{{', '.join(str(m) for m in mv)}}}, "
        f".growth_rate={s.get('growth_rate',0)} "
        f"}},  /* {s['name']} */"
    )

lines += ["};", ""]

dex_table = [0] * 256
for sid, dnum in SPECIES_TO_DEX.items():
    dex_table[sid] = dnum

lines.append("const uint8_t gSpeciesToDex[256] = {")
for i in range(0, 256, 16):
    row = ", ".join(f"{dex_table[j]:3d}" for j in range(i, min(i+16, 256)))
    lines.append(f"    /* 0x{i:02X} */ {row},")
lines += ["};", ""]

wfile("base_stats.c", "\n".join(lines))

# ---------------------------------------------------------------------------
# moves_data.h
# ---------------------------------------------------------------------------

wfile("moves_data.h", """\
#pragma once
/* moves_data.h -- Generated from pokered-master/data/moves/moves.asm */
#include <stdint.h>
#include "game/types.h"

#define NUM_MOVES 166

/* Indexed by move ID (0..165) */
extern const move_t gMoves[NUM_MOVES];
""")

# ---------------------------------------------------------------------------
# moves_data.c
# ---------------------------------------------------------------------------

# (anim==id, effect, power, type, accuracy scaled 0-255 from percent, pp)
MOVES_DATA = [
    (0x00,0x00,  0,0x00,  0,  0),  # NO_MOVE
    (0x01,0x00, 40,0x00,255, 35),  # POUND
    (0x02,0x00, 50,0x00,255, 25),  # KARATE_CHOP
    (0x03,0x1D, 15,0x00,217, 10),  # DOUBLESLAP
    (0x04,0x1D, 18,0x00,217, 15),  # COMET_PUNCH
    (0x05,0x00, 80,0x00,217, 20),  # MEGA_PUNCH
    (0x06,0x10, 40,0x00,255, 20),  # PAY_DAY
    (0x07,0x04, 75,0x14,255, 15),  # FIRE_PUNCH
    (0x08,0x05, 75,0x19,255, 15),  # ICE_PUNCH
    (0x09,0x06, 75,0x17,255, 15),  # THUNDERPUNCH
    (0x0A,0x00, 40,0x00,255, 35),  # SCRATCH
    (0x0B,0x00, 55,0x00,255, 30),  # VICEGRIP
    (0x0C,0x1F,  1,0x00, 77,  5),  # GUILLOTINE
    (0x0D,0x20, 80,0x00,191, 10),  # RAZOR_WIND
    (0x0E,0x2B,  0,0x00,255, 30),  # SWORDS_DANCE
    (0x0F,0x00, 50,0x00,243, 30),  # CUT
    (0x10,0x00, 40,0x00,255, 35),  # GUST
    (0x11,0x00, 35,0x02,255, 35),  # WING_ATTACK
    (0x12,0x1C,  0,0x00,217, 20),  # WHIRLWIND
    (0x13,0x24, 70,0x02,243, 15),  # FLY
    (0x14,0x23, 15,0x00,191, 20),  # BIND
    (0x15,0x00, 80,0x00,191, 20),  # SLAM
    (0x16,0x00, 35,0x16,255, 10),  # VINE_WHIP
    (0x17,0x43, 65,0x00,255, 20),  # STOMP
    (0x18,0x25, 30,0x01,255, 30),  # DOUBLE_KICK
    (0x19,0x00,120,0x00,191,  5),  # MEGA_KICK
    (0x1A,0x26, 70,0x01,243, 25),  # JUMP_KICK
    (0x1B,0x43, 60,0x01,217, 15),  # ROLLING_KICK
    (0x1C,0x16,  0,0x00,255, 15),  # SAND_ATTACK
    (0x1D,0x43, 70,0x00,255, 15),  # HEADBUTT
    (0x1E,0x00, 65,0x00,255, 25),  # HORN_ATTACK
    (0x1F,0x1D, 15,0x00,217, 20),  # FURY_ATTACK
    (0x20,0x1F,  1,0x00, 77,  5),  # HORN_DRILL
    (0x21,0x00, 35,0x00,243, 35),  # TACKLE
    (0x22,0x42, 85,0x00,255, 15),  # BODY_SLAM
    (0x23,0x23, 15,0x00,217, 20),  # WRAP
    (0x24,0x29, 90,0x00,217, 20),  # TAKE_DOWN
    (0x25,0x1B, 90,0x00,255, 20),  # THRASH
    (0x26,0x29,100,0x00,255, 15),  # DOUBLE_EDGE
    (0x27,0x13,  0,0x00,255, 30),  # TAIL_WHIP
    (0x28,0x02, 15,0x03,255, 35),  # POISON_STING
    (0x29,0x45, 25,0x07,255, 20),  # TWINEEDLE
    (0x2A,0x1D, 14,0x07,217, 20),  # PIN_MISSILE
    (0x2B,0x13,  0,0x00,255, 30),  # LEER
    (0x2C,0x1E, 60,0x00,255, 25),  # BITE
    (0x2D,0x12,  0,0x00,255, 40),  # GROWL
    (0x2E,0x1C,  0,0x00,255, 20),  # ROAR
    (0x2F,0x01,  0,0x00,140, 15),  # SING
    (0x30,0x2A,  0,0x00,140, 20),  # SUPERSONIC
    (0x31,0x22,  1,0x00,230, 20),  # SONICBOOM
    (0x32,0x4D,  0,0x00,140, 20),  # DISABLE
    (0x33,0x3D, 40,0x03,255, 30),  # ACID
    (0x34,0x04, 40,0x14,255, 25),  # EMBER
    (0x35,0x04, 95,0x14,255, 15),  # FLAMETHROWER
    (0x36,0x27,  0,0x19,255, 30),  # MIST
    (0x37,0x00, 40,0x15,255, 25),  # WATER_GUN
    (0x38,0x00,120,0x15,204,  5),  # HYDRO_PUMP
    (0x39,0x00, 95,0x15,255, 15),  # SURF
    (0x3A,0x05, 95,0x19,255, 10),  # ICE_BEAM
    (0x3B,0x05,120,0x19,230,  5),  # BLIZZARD
    (0x3C,0x44, 65,0x18,255, 20),  # PSYBEAM
    (0x3D,0x3E, 65,0x15,255, 20),  # BUBBLEBEAM
    (0x3E,0x3C, 65,0x19,255, 20),  # AURORA_BEAM
    (0x3F,0x47,150,0x00,230,  5),  # HYPER_BEAM
    (0x40,0x00, 35,0x02,255, 35),  # PECK
    (0x41,0x00, 80,0x02,255, 20),  # DRILL_PECK
    (0x42,0x29, 80,0x01,204, 25),  # SUBMISSION
    (0x43,0x43, 50,0x01,230, 20),  # LOW_KICK
    (0x44,0x00,  1,0x01,255, 20),  # COUNTER
    (0x45,0x22,  1,0x01,255, 20),  # SEISMIC_TOSS
    (0x46,0x00, 80,0x00,255, 15),  # STRENGTH
    (0x47,0x03, 20,0x16,255, 20),  # ABSORB
    (0x48,0x03, 40,0x16,255, 10),  # MEGA_DRAIN
    (0x49,0x4B,  0,0x16,230, 10),  # LEECH_SEED
    (0x4A,0x0D,  0,0x00,255, 40),  # GROWTH
    (0x4B,0x00, 55,0x16,243, 25),  # RAZOR_LEAF
    (0x4C,0x20,120,0x16,255, 10),  # SOLARBEAM
    (0x4D,0x3A,  0,0x03,191, 35),  # POISONPOWDER
    (0x4E,0x3B,  0,0x16,191, 30),  # STUN_SPORE
    (0x4F,0x01,  0,0x16,191, 15),  # SLEEP_POWDER
    (0x50,0x1B, 70,0x16,255, 20),  # PETAL_DANCE
    (0x51,0x14,  0,0x07,243, 40),  # STRING_SHOT
    (0x52,0x22,  1,0x1A,255, 10),  # DRAGON_RAGE
    (0x53,0x23, 15,0x14,179, 15),  # FIRE_SPIN
    (0x54,0x06, 40,0x17,255, 30),  # THUNDERSHOCK
    (0x55,0x06, 95,0x17,255, 15),  # THUNDERBOLT
    (0x56,0x3B,  0,0x17,255, 20),  # THUNDER_WAVE
    (0x57,0x06,120,0x17,179, 10),  # THUNDER
    (0x58,0x00, 50,0x05,166, 15),  # ROCK_THROW
    (0x59,0x00,100,0x04,255, 10),  # EARTHQUAKE
    (0x5A,0x1F,  1,0x04, 77,  5),  # FISSURE
    (0x5B,0x20,100,0x04,255, 10),  # DIG
    (0x5C,0x3A,  0,0x03,217, 10),  # TOXIC
    (0x5D,0x44, 50,0x18,255, 25),  # CONFUSION
    (0x5E,0x3F, 90,0x18,255, 10),  # PSYCHIC_M
    (0x5F,0x01,  0,0x18,153, 20),  # HYPNOSIS
    (0x60,0x0A,  0,0x18,255, 40),  # MEDITATE
    (0x61,0x0C,  0,0x18,255, 30),  # AGILITY
    (0x62,0x00, 40,0x00,255, 30),  # QUICK_ATTACK
    (0x63,0x48, 20,0x00,255, 20),  # RAGE
    (0x64,0x1C,  0,0x18,255, 20),  # TELEPORT
    (0x65,0x22,  0,0x08,255, 15),  # NIGHT_SHADE
    (0x66,0x49,  0,0x00,255, 10),  # MIMIC
    (0x67,0x33,  0,0x00,217, 40),  # SCREECH
    (0x68,0x0F,  0,0x00,255, 15),  # DOUBLE_TEAM
    (0x69,0x30,  0,0x00,255, 20),  # RECOVER
    (0x6A,0x0B,  0,0x00,255, 30),  # HARDEN
    (0x6B,0x0F,  0,0x00,255, 20),  # MINIMIZE
    (0x6C,0x16,  0,0x00,255, 20),  # SMOKESCREEN
    (0x6D,0x2A,  0,0x08,255, 10),  # CONFUSE_RAY
    (0x6E,0x0B,  0,0x15,255, 40),  # WITHDRAW
    (0x6F,0x0B,  0,0x00,255, 40),  # DEFENSE_CURL
    (0x70,0x2C,  0,0x18,255, 30),  # BARRIER
    (0x71,0x38,  0,0x18,255, 30),  # LIGHT_SCREEN
    (0x72,0x19,  0,0x19,255, 30),  # HAZE
    (0x73,0x39,  0,0x18,255, 20),  # REFLECT
    (0x74,0x28,  0,0x00,255, 30),  # FOCUS_ENERGY
    (0x75,0x1A,  0,0x00,255, 10),  # BIDE
    (0x76,0x4A,  0,0x00,255, 10),  # METRONOME
    (0x77,0x09,  0,0x02,255, 20),  # MIRROR_MOVE
    (0x78,0x07,130,0x00,255,  5),  # SELFDESTRUCT
    (0x79,0x00,100,0x00,191, 10),  # EGG_BOMB
    (0x7A,0x42, 20,0x08,255, 30),  # LICK
    (0x7B,0x4E, 20,0x03,179, 20),  # SMOG
    (0x7C,0x4E, 65,0x03,255, 20),  # SLUDGE
    (0x7D,0x1E, 65,0x04,217, 20),  # BONE_CLUB
    (0x7E,0x40,120,0x14,217,  5),  # FIRE_BLAST
    (0x7F,0x00, 80,0x15,255, 15),  # WATERFALL
    (0x80,0x23, 35,0x15,191, 10),  # CLAMP
    (0x81,0x11, 60,0x00,255, 20),  # SWIFT
    (0x82,0x20,100,0x00,255, 15),  # SKULL_BASH
    (0x83,0x1D, 20,0x00,255, 15),  # SPIKE_CANNON
    (0x84,0x3E, 10,0x00,255, 35),  # CONSTRICT
    (0x85,0x2D,  0,0x18,255, 20),  # AMNESIA
    (0x86,0x16,  0,0x18,204, 15),  # KINESIS
    (0x87,0x30,  0,0x00,255, 10),  # SOFTBOILED
    (0x88,0x26, 85,0x01,230, 20),  # HI_JUMP_KICK
    (0x89,0x3B,  0,0x00,191, 30),  # GLARE
    (0x8A,0x08,100,0x18,255, 15),  # DREAM_EATER
    (0x8B,0x3A,  0,0x03,140, 40),  # POISON_GAS
    (0x8C,0x1D, 15,0x00,217, 20),  # BARRAGE
    (0x8D,0x03, 20,0x07,255, 15),  # LEECH_LIFE
    (0x8E,0x01,  0,0x00,191, 10),  # LOVELY_KISS
    (0x8F,0x20,140,0x02,230,  5),  # SKY_ATTACK
    (0x90,0x31,  0,0x00,255, 10),  # TRANSFORM
    (0x91,0x3E, 20,0x15,255, 30),  # BUBBLE
    (0x92,0x00, 70,0x00,255, 10),  # DIZZY_PUNCH
    (0x93,0x01,  0,0x16,255, 15),  # SPORE
    (0x94,0x16,  0,0x00,179, 20),  # FLASH
    (0x95,0x22,  1,0x18,204, 15),  # PSYWAVE
    (0x96,0x4C,  0,0x00,255, 40),  # SPLASH
    (0x97,0x2C,  0,0x03,255, 40),  # ACID_ARMOR
    (0x98,0x00, 90,0x15,217, 10),  # CRABHAMMER
    (0x99,0x07,170,0x00,255,  5),  # EXPLOSION
    (0x9A,0x1D, 18,0x00,204, 15),  # FURY_SWIPES
    (0x9B,0x25, 50,0x04,230, 10),  # BONEMERANG
    (0x9C,0x30,  0,0x18,255, 10),  # REST
    (0x9D,0x00, 75,0x05,230, 10),  # ROCK_SLIDE
    (0x9E,0x1E, 80,0x00,230, 15),  # HYPER_FANG
    (0x9F,0x0A,  0,0x00,255, 30),  # SHARPEN
    (0xA0,0x18,  0,0x00,255, 30),  # CONVERSION
    (0xA1,0x00, 80,0x00,255, 10),  # TRI_ATTACK
    (0xA2,0x21,  1,0x00,230, 10),  # SUPER_FANG
    (0xA3,0x00, 70,0x00,255, 20),  # SLASH
    (0xA4,0x46,  0,0x00,255, 10),  # SUBSTITUTE
    (0xA5,0x29, 50,0x00,255, 10),  # STRUGGLE
]

move_names_list = list(MOVE_IDS.keys())

moves_lines = [
    "/* moves_data.c -- Generated from pokered-master/data/moves/moves.asm */",
    '#include "moves_data.h"',
    "",
    "const move_t gMoves[NUM_MOVES] = {",
]
for i, (anim, effect, power, mtype, acc, pp) in enumerate(MOVES_DATA):
    nm = move_names_list[i] if i < len(move_names_list) else f"MOVE_{i}"
    moves_lines.append(
        f"    [{i}] = {{ .anim=0x{anim:02X}, .effect=0x{effect:02X}, "
        f".power={power:3d}, .type=0x{mtype:02X}, "
        f".accuracy={acc:3d}, .pp={pp:2d} }},  /* {nm} */"
    )
moves_lines += ["};", ""]

wfile("moves_data.c", "\n".join(moves_lines))

# ---------------------------------------------------------------------------
# type_chart.h
# ---------------------------------------------------------------------------

wfile("type_chart.h", """\
#pragma once
/* type_chart.h -- Type effectiveness lookup.
 *
 * TypeEffectiveness(attacker_type, defender_type) returns:
 *   0  = no effect
 *   5  = not very effective
 *  10  = normal (default)
 *  20  = super effective
 *
 * Type byte values: NORMAL=0x00 FIGHTING=0x01 FLYING=0x02 POISON=0x03
 *   GROUND=0x04 ROCK=0x05 BUG=0x07 GHOST=0x08 FIRE=0x14 WATER=0x15
 *   GRASS=0x16 ELECTRIC=0x17 PSYCHIC=0x18 ICE=0x19 DRAGON=0x1A
 */
#include <stdint.h>

uint8_t TypeEffectiveness(uint8_t attacker, uint8_t defender);
""")

# ---------------------------------------------------------------------------
# type_chart.c
# ---------------------------------------------------------------------------

TYPE_CHART_ENTRIES = [
    (0x15,0x14,20),(0x14,0x16,20),(0x14,0x19,20),
    (0x16,0x15,20),(0x17,0x15,20),(0x15,0x05,20),
    (0x04,0x02, 0),(0x15,0x15, 5),(0x14,0x14, 5),
    (0x17,0x17, 5),(0x19,0x19, 5),(0x16,0x16, 5),
    (0x18,0x18, 5),(0x14,0x15, 5),(0x16,0x14, 5),
    (0x15,0x16, 5),(0x17,0x16, 5),(0x00,0x05, 5),
    (0x00,0x08, 0),(0x08,0x08,20),(0x14,0x07,20),
    (0x14,0x05, 5),(0x15,0x04,20),(0x17,0x04, 0),
    (0x17,0x02,20),(0x16,0x04,20),(0x16,0x07, 5),
    (0x16,0x03, 5),(0x16,0x05,20),(0x16,0x02, 5),
    (0x19,0x15, 5),(0x19,0x16,20),(0x19,0x04,20),
    (0x19,0x02,20),(0x01,0x00,20),(0x01,0x03, 5),
    (0x01,0x02, 5),(0x01,0x18, 5),(0x01,0x07, 5),
    (0x01,0x05,20),(0x01,0x19,20),(0x01,0x08, 0),
    (0x03,0x16,20),(0x03,0x03, 5),(0x03,0x04, 5),
    (0x03,0x07,20),(0x03,0x05, 5),(0x03,0x08, 5),
    (0x04,0x14,20),(0x04,0x17,20),(0x04,0x16, 5),
    (0x04,0x07, 5),(0x04,0x05,20),(0x04,0x03,20),
    (0x02,0x17, 5),(0x02,0x01,20),(0x02,0x07,20),
    (0x02,0x16,20),(0x02,0x05, 5),(0x18,0x01,20),
    (0x18,0x03,20),(0x07,0x14, 5),(0x07,0x16,20),
    (0x07,0x01, 5),(0x07,0x02, 5),(0x07,0x18,20),
    (0x07,0x08, 5),(0x07,0x03,20),(0x05,0x14,20),
    (0x05,0x01, 5),(0x05,0x04, 5),(0x05,0x02,20),
    (0x05,0x07,20),(0x05,0x19,20),(0x08,0x00, 0),
    (0x08,0x18, 0),(0x14,0x1A, 5),(0x15,0x1A, 5),
    (0x17,0x1A, 5),(0x16,0x1A, 5),(0x19,0x1A,20),
    (0x1A,0x1A,20),
]

tc_lines = [
    "/* type_chart.c -- Generated from pokered-master/data/types/type_matchups.asm */",
    '#include "type_chart.h"',
    "",
    "typedef struct { uint8_t atk, def, eff; } type_entry_t;",
    "",
    "static const type_entry_t kTypeChart[] = {",
]
for atk, def_, eff in TYPE_CHART_ENTRIES:
    tc_lines.append(f"    {{ 0x{atk:02X}, 0x{def_:02X}, {eff:2d} }},")
tc_lines += [
    "    { 0xFF, 0xFF,  0 } /* sentinel */",
    "};",
    "",
    "uint8_t TypeEffectiveness(uint8_t attacker, uint8_t defender) {",
    "    for (int i = 0; kTypeChart[i].atk != 0xFF; i++)",
    "        if (kTypeChart[i].atk == attacker && kTypeChart[i].def == defender)",
    "            return kTypeChart[i].eff;",
    "    return 10; /* normal damage */",
    "}",
    "",
]

wfile("type_chart.c", "\n".join(tc_lines))

print("Done.")
