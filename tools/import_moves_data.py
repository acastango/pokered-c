#!/usr/bin/env python3
"""
Import move data directly from pokered ASM source of truth.

Generates:
  src/data/moves_data.c
"""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(r"C:/Users/Anthony/pokered")
ASM = ROOT / "pokered-master"
OUT = ROOT / "src/data/moves_data.c"


TYPE_IDS = {
    "NORMAL": 0x00,
    "FIGHTING": 0x01,
    "FLYING": 0x02,
    "POISON": 0x03,
    "GROUND": 0x04,
    "ROCK": 0x05,
    "BIRD": 0x06,
    "BUG": 0x07,
    "GHOST": 0x08,
    "FIRE": 0x14,
    "WATER": 0x15,
    "GRASS": 0x16,
    "ELECTRIC": 0x17,
    "PSYCHIC_TYPE": 0x18,
    "ICE": 0x19,
    "DRAGON": 0x1A,
}


MOVE_RE = re.compile(
    r"^\s*move\s+([A-Z0-9_]+)\s*,\s*([A-Z0-9_]+)\s*,\s*([0-9]+)\s*,\s*([A-Z0-9_]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)"
)


def strip_comment(line: str) -> str:
    i = line.find(";")
    if i >= 0:
        return line[:i]
    return line


def parse_effect_constants(path: Path) -> dict[str, int]:
    effect_ids: dict[str, int] = {}
    value = 0
    started = False

    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = strip_comment(raw).strip()
        if not line:
            continue
        if "const_def" in line:
            started = True
            value = 0
            continue
        if not started:
            continue
        if line.startswith("const_skip"):
            value += 1
            continue
        if line.startswith("const "):
            parts = line.split()
            if len(parts) >= 2:
                effect_ids[parts[1]] = value
                value += 1
            continue
        # Stop after the effect table block.
        if line.startswith("DEF NUM_MOVE_EFFECTS"):
            break
    return effect_ids


def acc_percent_to_byte(percent: int) -> int:
    # RGBDS "percent" macro equivalent for these data tables.
    return (percent * 255) // 100


def main() -> None:
    effects = parse_effect_constants(ASM / "constants/move_effect_constants.asm")
    if not effects:
        raise RuntimeError("Failed to parse move effect constants")

    moves: list[tuple[str, int, int, int, int]] = []
    for raw in (ASM / "data/moves/moves.asm").read_text(encoding="utf-8", errors="replace").splitlines():
        line = strip_comment(raw)
        m = MOVE_RE.match(line)
        if not m:
            continue
        name, effect_sym, power_s, type_sym, acc_percent_s, pp_s = m.groups()
        if effect_sym not in effects:
            raise RuntimeError(f"Unknown effect symbol {effect_sym} for move {name}")
        if type_sym not in TYPE_IDS:
            raise RuntimeError(f"Unknown type symbol {type_sym} for move {name}")

        power = int(power_s)
        acc_percent = int(acc_percent_s)
        pp = int(pp_s)
        moves.append((name, effects[effect_sym], power, TYPE_IDS[type_sym], acc_percent_to_byte(acc_percent), pp))

    if len(moves) != 165:
        raise RuntimeError(f"Expected 165 moves from ASM, got {len(moves)}")

    # Parse display names from names.asm (li "...")
    move_names: list[str] = []
    for raw in (ASM / "data/moves/names.asm").read_text(encoding="utf-8", errors="replace").splitlines():
        line = strip_comment(raw).strip()
        m = re.match(r'^li\s+"([^"]+)"', line)
        if m:
            move_names.append(m.group(1))
    if len(move_names) != 165:
        raise RuntimeError(f"Expected 165 move names from ASM, got {len(move_names)}")

    lines: list[str] = []
    lines.append("/* moves_data.c -- Generated from pokered-master/data/moves/moves.asm */")
    lines.append('#include "moves_data.h"')
    lines.append("")
    lines.append("const move_t gMoves[NUM_MOVE_DEFS] = {")
    lines.append("    [0] = { .anim=0x00, .effect=0x00, .power=  0, .type=0x00, .accuracy=  0, .pp= 0 },  /* NO_MOVE */")

    for i, (name, effect, power, mtype, acc, pp) in enumerate(moves, start=1):
        lines.append(
            f"    [{i}] = {{ .anim=0x{i:02X}, .effect=0x{effect:02X}, "
            f".power={power:3d}, .type=0x{mtype:02X}, .accuracy={acc:3d}, .pp={pp:2d} }},  /* {name} */"
        )

    lines.append("};")
    lines.append("")
    lines.append("const char * const gMoveNames[NUM_MOVE_DEFS] = {")
    lines.append('    [0] = "------",')
    for i, nm in enumerate(move_names, start=1):
        esc = nm.replace("\\", "\\\\").replace('"', '\\"')
        lines.append(f'    [{i}] = "{esc}",')
    lines.append("};")
    lines.append("")
    OUT.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {OUT}")


if __name__ == "__main__":
    main()
