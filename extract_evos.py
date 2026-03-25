#!/usr/bin/env python3
"""Extract evos+moves data from pokered-master ASM and generate C data files."""
import re

BASE = "/c/Users/Anthony/pokered/pokered-master"
OUT_C = "/c/Users/Anthony/pokered/src/data/evos_moves_data.c"
OUT_H = "/c/Users/Anthony/pokered/src/data/evos_moves_data.h"

# 1. Parse move constants
move_ids = {}
with open(f"{BASE}/constants/move_constants.asm") as f:
    idx = -1
    for line in f:
        line = line.strip()
        if line == "const_def":
            idx = 0
            continue
        m = re.match(r'const\s+(\w+)', line)
        if m and idx >= 0:
            move_ids[m.group(1)] = idx
            idx += 1

# 2. Parse pokemon constants (with const_skip for gaps)
species_ids = {}
with open(f"{BASE}/constants/pokemon_constants.asm") as f:
    idx = -1
    for line in f:
        line = line.strip()
        if line == "const_def":
            idx = 0
            continue
        if idx < 0:
            continue
        m = re.match(r'const\s+(\w+)', line)
        if m:
            species_ids[m.group(1)] = idx
            idx += 1
        elif re.match(r'const_skip', line):
            idx += 1

KNOWN_CONSTANTS = {
    'EVOLVE_LEVEL': 1, 'EVOLVE_ITEM': 2, 'EVOLVE_TRADE': 3,
    'GROWTH_MEDIUM_FAST': 0, 'GROWTH_SLIGHTLY_FAST': 1,
    'GROWTH_SLIGHTLY_SLOW': 2, 'GROWTH_MEDIUM_SLOW': 3,
    'GROWTH_FAST': 4, 'GROWTH_SLOW': 5,
}

def expand_token(tok):
    tok = tok.strip()
    if tok.startswith('$'):
        return int(tok[1:], 16)
    if tok.isdigit():
        return int(tok)
    if tok in KNOWN_CONSTANTS:
        return KNOWN_CONSTANTS[tok]
    if tok in move_ids:
        return move_ids[tok]
    if tok in species_ids:
        return species_ids[tok]
    return None

# 3. Read evos_moves.asm
with open(f"{BASE}/data/pokemon/evos_moves.asm") as f:
    lines = f.readlines()

# 4. Extract pointer table order
ptr_table = []
in_table = False
for line in lines:
    stripped = line.strip()
    if stripped == "EvosMovesPointerTable:":
        in_table = True
        continue
    if in_table:
        m = re.match(r'dw\s+(\w+)', stripped)
        if m:
            ptr_table.append(m.group(1))
        elif stripped.startswith("assert_table_length"):
            break

print(f"Pointer table: {len(ptr_table)} entries")

# 5. Parse each species block
EVOLVE_LEVEL = 1
EVOLVE_ITEM  = 2
EVOLVE_TRADE = 3

blocks = {}  # label -> (evo_list, learnset_list)
current_label = None
current_evos = []
current_learnset = []
in_evos = True

for line in lines:
    stripped = line.strip()
    # Label
    m = re.match(r'^(\w+EvosMoves):', stripped)
    if m:
        if current_label:
            blocks[current_label] = (current_evos[:], current_learnset[:])
        current_label = m.group(1)
        current_evos = []
        current_learnset = []
        in_evos = True
        continue
    if not stripped or stripped.startswith(';'):
        continue
    # Remove inline comment
    code = re.sub(r'\s*;.*', '', stripped).strip()
    if not code:
        continue
    m = re.match(r'db\s+(.+)', code)
    if m and current_label:
        args_raw = m.group(1).split(',')
        args = [a.strip() for a in args_raw]
        # Check terminator
        if len(args) == 1 and (args[0] == '0' or args[0] == '$0'):
            if in_evos:
                in_evos = False
            continue
        first = expand_token(args[0])
        if first is None:
            continue
        if in_evos:
            if first == EVOLVE_LEVEL and len(args) >= 3:
                level = expand_token(args[1])
                target = expand_token(args[2])
                if level is not None and target is not None:
                    current_evos.append((EVOLVE_LEVEL, level, target))
            elif first == EVOLVE_ITEM and len(args) >= 4:
                item = expand_token(args[1])
                min_lv = expand_token(args[2])
                target = expand_token(args[3])
                if all(v is not None for v in [item, min_lv, target]):
                    current_evos.append((EVOLVE_ITEM, item, min_lv, target))
            elif first == EVOLVE_TRADE and len(args) >= 3:
                min_lv = expand_token(args[1])
                target = expand_token(args[2])
                if min_lv is not None and target is not None:
                    current_evos.append((EVOLVE_TRADE, min_lv, target))
        else:
            if len(args) == 2:
                move = expand_token(args[1])
                if move is not None:
                    current_learnset.append((first, move))

if current_label:
    blocks[current_label] = (current_evos[:], current_learnset[:])

print(f"Parsed {len(blocks)} blocks")
missing = [n for n in ptr_table if n not in blocks]
if missing:
    print(f"Missing: {missing}")

# Verify Bulbasaur
bev, bls = blocks.get('BulbasaurEvosMoves', ([], []))
print(f"Bulbasaur evos: {[(e[0], e[1], hex(e[2])) for e in bev]}")
print(f"Bulbasaur learnset: {[(lv, hex(mv)) for lv,mv in bls]}")

# 6. Generate raw byte arrays
def species_to_bytes(evos, learnset):
    out = []
    for evo in evos:
        if evo[0] == EVOLVE_LEVEL:
            out += [EVOLVE_LEVEL, evo[1], evo[2]]
        elif evo[0] == EVOLVE_ITEM:
            out += [EVOLVE_ITEM, evo[1], evo[2], evo[3]]
        elif evo[0] == EVOLVE_TRADE:
            out += [EVOLVE_TRADE, evo[1], evo[2]]
    out.append(0)  # evo terminator
    for level, move in learnset:
        out += [level, move]
    out.append(0)  # learnset terminator
    return out

all_arrays = []
for idx, label in enumerate(ptr_table):
    species_id = idx + 1
    evos, learnset = blocks.get(label, ([], []))
    data = species_to_bytes(evos, learnset)
    all_arrays.append((species_id, label, data))

# 7. Write C file
c_lines = [
    "/* evos_moves_data.c — Generated from pokered-master/data/pokemon/evos_moves.asm",
    " *",
    " * Byte format per species (mirrors evos_moves.asm):",
    " *   Evolution entries (variable length), terminated by 0x00",
    " *     EVOLVE_LEVEL: 0x01, level, target_species_id",
    " *     EVOLVE_ITEM:  0x02, item_id, min_level, target_species_id",
    " *     EVOLVE_TRADE: 0x03, min_level, target_species_id",
    " *   Learnset pairs (level, move_id), terminated by 0x00",
    " *",
    " * Table indexed by (internal_species_id - 1).",
    " */",
    "",
    '#include "evos_moves_data.h"',
    "",
]

for species_id, label, data in all_arrays:
    name = label.replace("EvosMoves", "")
    hex_bytes = ", ".join(f"0x{b:02X}" for b in data)
    c_lines.append(f"static const uint8_t k{name}EvosMoves[] = {{{hex_bytes}}};")

c_lines.append("")
c_lines.append(f"const uint8_t * const gEvosMoves[{len(all_arrays) + 1}] = {{")
c_lines.append("    /* [0] unused */ NULL,")
for species_id, label, data in all_arrays:
    name = label.replace("EvosMoves", "")
    c_lines.append(f"    /* [0x{species_id:02X}] */ k{name}EvosMoves,")
c_lines.append("};")
c_lines.append("")

with open(OUT_C, 'w') as f:
    f.write('\n'.join(c_lines))
print(f"\nWrote {OUT_C}")

# 8. Write H file
h_lines = [
    "#pragma once",
    "/* evos_moves_data.h — Evolution and learnset data for all species. */",
    "#include <stdint.h>",
    "#include <stddef.h>",
    "",
    "/* Evolution type constants (mirrors EVOLVE_* in pokemon_data_constants.asm) */",
    "#define EVOLVE_LEVEL  1",
    "#define EVOLVE_ITEM   2",
    "#define EVOLVE_TRADE  3",
    "",
    f"/* Table size: {len(all_arrays) + 1} entries (index 0 unused, 1..{len(all_arrays)} = internal species IDs) */",
    f"#define EVOS_MOVES_TABLE_SIZE  {len(all_arrays) + 1}",
    "",
    f"extern const uint8_t * const gEvosMoves[EVOS_MOVES_TABLE_SIZE];",
    "",
]

with open(OUT_H, 'w') as f:
    f.write('\n'.join(h_lines))
print(f"Wrote {OUT_H}")
print("Done.")
