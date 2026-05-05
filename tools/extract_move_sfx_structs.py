#!/usr/bin/env python3
"""
extract_move_sfx_structs.py

Build a canonical C data dump of move SFX structure from pokered ASM:
 - move -> SFX symbol (already in data/moves/sfx.asm)
 - SFX symbol -> bank/header channels (audio/headers/sfxheaders*.asm)
 - channel label -> command stream (audio/sfx/*.asm)

Outputs:
 - src/data/move_sfx_structs.h
 - src/data/move_sfx_structs.c
"""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(r"C:\Users\Anthony\pokered")
MOVE_SFX_ASM = ROOT / "pokered-master" / "data" / "moves" / "sfx.asm"
SFX_HEADERS = [
    ROOT / "pokered-master" / "audio" / "headers" / "sfxheaders1.asm",
    ROOT / "pokered-master" / "audio" / "headers" / "sfxheaders2.asm",
    ROOT / "pokered-master" / "audio" / "headers" / "sfxheaders3.asm",
]
SFX_DIR = ROOT / "pokered-master" / "audio" / "sfx"
OUT_H = ROOT / "src" / "data" / "move_sfx_structs.h"
OUT_C = ROOT / "src" / "data" / "move_sfx_structs.c"

MOVE_ROW_RE = re.compile(
    r"^\s*db\s+([A-Z0-9_]+)\s*,\s*\$([0-9A-Fa-f]{2})\s*,\s*\$([0-9A-Fa-f]{2})"
)
LABEL_RE = re.compile(r"^([A-Za-z0-9_\.]+):{1,2}$")
CHAN_COUNT_RE = re.compile(r"^\s*channel_count\s+(\d+)\s*$")
CHAN_RE = re.compile(r"^\s*channel\s+(\d+)\s*,\s*([A-Za-z0-9_\.]+)\s*$")

CMD_PATTERNS = [
    ("DUTY_CYCLE", re.compile(r"^\s*duty_cycle\s+(-?\d+)\s*$")),
    ("DUTY_CYCLE_PATTERN", re.compile(r"^\s*duty_cycle_pattern\s+(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*$")),
    ("PITCH_SWEEP", re.compile(r"^\s*pitch_sweep\s+(-?\d+)\s*,\s*(-?\d+)\s*$")),
    ("SQUARE_NOTE", re.compile(r"^\s*square_note\s+(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*$")),
    ("NOISE_NOTE", re.compile(r"^\s*noise_note\s+(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*$")),
    ("SOUND_LOOP", re.compile(r"^\s*sound_loop\s+(-?\d+)\s*,\s*([A-Za-z0-9_\.]+)\s*$")),
    ("SOUND_RET", re.compile(r"^\s*sound_ret\s*$")),
]


def strip_comment(line: str) -> str:
    return line.split(";", 1)[0].strip()


def parse_move_symbols() -> list[str]:
    symbols: list[str] = []
    for raw in MOVE_SFX_ASM.read_text(encoding="utf-8").splitlines():
        m = MOVE_ROW_RE.match(strip_comment(raw))
        if m:
            sym = m.group(1)
            if sym not in symbols:
                symbols.append(sym)
    return symbols


def parse_header_map() -> dict[str, dict]:
    out: dict[str, dict] = {}
    for bank_index, header in enumerate(SFX_HEADERS, start=1):
        lines = header.read_text(encoding="utf-8").splitlines()
        i = 0
        while i < len(lines):
            label_m = LABEL_RE.match(strip_comment(lines[i]))
            if not label_m:
                i += 1
                continue
            label = label_m.group(1)
            i += 1
            if i >= len(lines):
                break
            count_m = CHAN_COUNT_RE.match(strip_comment(lines[i]))
            if not count_m:
                continue
            chan_count = int(count_m.group(1))
            i += 1
            channels: list[tuple[int, str]] = []
            for _ in range(chan_count):
                if i >= len(lines):
                    break
                chan_m = CHAN_RE.match(strip_comment(lines[i]))
                if chan_m:
                    channels.append((int(chan_m.group(1)), chan_m.group(2)))
                i += 1
            out[label] = {"bank": bank_index, "channels": channels}
    return out


def norm_symbol(s: str) -> str:
    return re.sub(r"[^A-Z0-9_]", "_", s.upper())


def load_sfx_files() -> list[tuple[Path, list[str]]]:
    out: list[tuple[Path, list[str]]] = []
    for p in sorted(SFX_DIR.glob("*.asm")):
        out.append((p, p.read_text(encoding="utf-8").splitlines()))
    return out


def find_channel_commands(label: str, files: list[tuple[Path, list[str]]]) -> list[tuple[str, list[int]]]:
    for _, lines in files:
        start = -1
        for idx, raw in enumerate(lines):
            lm = LABEL_RE.match(strip_comment(raw))
            if lm and lm.group(1) == label:
                start = idx + 1
                break
        if start < 0:
            continue
        cmds: list[tuple[str, list[int]]] = []
        local_labels: dict[str, int] = {}
        pending_loops: list[tuple[int, int, str]] = []
        for j in range(start, len(lines)):
            s = strip_comment(lines[j])
            if not s:
                continue
            lm = LABEL_RE.match(s)
            if lm:
                lbl = lm.group(1)
                if lbl == label:
                    continue
                local_labels[lbl] = len(cmds)
                continue
            matched = False
            for name, pat in CMD_PATTERNS:
                m = pat.match(s)
                if not m:
                    continue
                params = list(m.groups()) if m.groups() else []
                if name in ("DUTY_CYCLE",):
                    cmds.append((name, [int(params[0])]))
                elif name in ("DUTY_CYCLE_PATTERN", "SQUARE_NOTE", "NOISE_NOTE"):
                    cmds.append((name, [int(params[0]), int(params[1]), int(params[2]), int(params[3])]))
                elif name == "PITCH_SWEEP":
                    cmds.append((name, [int(params[0]), int(params[1])]))
                elif name == "SOUND_LOOP":
                    # p0=count, p1=target cmd index (resolved after scan)
                    cmds.append((name, [int(params[0]), -1]))
                    pending_loops.append((len(cmds) - 1, int(params[0]), params[1]))
                else:
                    cmds.append((name, []))
                matched = True
                break
            if matched and cmds[-1][0] == "SOUND_RET":
                break
        for cmd_idx, count, target_lbl in pending_loops:
            target = local_labels.get(target_lbl, 0)
            cmds[cmd_idx] = ("SOUND_LOOP", [count, target])
        return cmds
    return []


def c_ident(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", s)


def emit(symbols: list[str], header_map: dict[str, dict], files: list[tuple[Path, list[str]]]) -> tuple[str, str]:
    h_lines = [
        "#pragma once",
        "/* move_sfx_structs.h -- AUTO-GENERATED by tools/extract_move_sfx_structs.py. */",
        "#include <stdint.h>",
        "",
        "typedef enum {",
        "    MOVE_SFX_CMD_DUTY_CYCLE = 0,",
        "    MOVE_SFX_CMD_DUTY_CYCLE_PATTERN = 1,",
        "    MOVE_SFX_CMD_PITCH_SWEEP = 2,",
        "    MOVE_SFX_CMD_SQUARE_NOTE = 3,",
        "    MOVE_SFX_CMD_NOISE_NOTE = 4,",
        "    MOVE_SFX_CMD_SOUND_LOOP = 5,",
        "    MOVE_SFX_CMD_SOUND_RET = 6,",
        "} move_sfx_cmd_type_t;",
        "",
        "typedef struct {",
        "    uint8_t type;",
        "    int16_t p0;",
        "    int16_t p1;",
        "    int16_t p2;",
        "    int16_t p3;",
        "} move_sfx_cmd_t;",
        "",
        "typedef struct {",
        "    uint8_t hw_channel;",
        "    const char *label;",
        "    const move_sfx_cmd_t *cmds;",
        "    uint16_t cmd_count;",
        "} move_sfx_channel_def_t;",
        "",
        "typedef struct {",
        "    const char *symbol;",
        "    uint8_t bank;",
        "    const move_sfx_channel_def_t *channels;",
        "    uint8_t channel_count;",
        "} move_sfx_def_t;",
        "",
        "extern const move_sfx_def_t gMoveSfxStructs[];",
        "extern const uint16_t gMoveSfxStructCount;",
        "",
    ]

    c_lines = [
        "/* move_sfx_structs.c -- AUTO-GENERATED by tools/extract_move_sfx_structs.py. */",
        '#include "move_sfx_structs.h"',
        "",
    ]

    def_entries: list[str] = []

    header_norm = {norm_symbol(k): v for k, v in header_map.items()}

    for sym in symbols:
        meta = header_norm.get(norm_symbol(sym))
        if meta is None:
            continue
        bank = meta["bank"]
        channels: list[tuple[int, str]] = meta["channels"]
        chan_arr_name = f"sfx_{c_ident(sym)}_channels"
        chan_items: list[str] = []

        for hw_ch, label in channels:
            cmds = find_channel_commands(label, files)
            cmd_arr_name = f"sfx_{c_ident(label)}_cmds"
            c_lines.append(f"static const move_sfx_cmd_t {cmd_arr_name}[] = {{")
            for cmd_name, params in cmds:
                p = [0, 0, 0, 0]
                if cmd_name == "DUTY_CYCLE":
                    p[0] = params[0]
                elif cmd_name == "DUTY_CYCLE_PATTERN":
                    p = [params[0], params[1], params[2], params[3]]
                elif cmd_name == "PITCH_SWEEP":
                    p[0] = params[0]; p[1] = params[1]
                elif cmd_name == "SQUARE_NOTE":
                    p = [params[0], params[1], params[2], params[3]]
                elif cmd_name == "NOISE_NOTE":
                    p = [params[0], params[1], params[2], params[3]]
                elif cmd_name == "SOUND_LOOP":
                    p[0] = params[0]
                    p[1] = params[1]
                c_lines.append(
                    f"    {{ MOVE_SFX_CMD_{cmd_name}, {p[0]}, {p[1]}, {p[2]}, {p[3]} }},"
                )
            c_lines.append("};")
            c_lines.append("")
            chan_items.append(
                f'    {{ {hw_ch}, "{label}", {cmd_arr_name}, (uint16_t)(sizeof({cmd_arr_name})/sizeof({cmd_arr_name}[0])) }},'
            )

        c_lines.append(f"static const move_sfx_channel_def_t {chan_arr_name}[] = {{")
        c_lines.extend(chan_items)
        c_lines.append("};")
        c_lines.append("")
        def_entries.append(
            f'    {{ "{sym}", {bank}, {chan_arr_name}, (uint8_t)(sizeof({chan_arr_name})/sizeof({chan_arr_name}[0])) }},'
        )

    c_lines.append("const move_sfx_def_t gMoveSfxStructs[] = {")
    c_lines.extend(def_entries)
    c_lines.append("};")
    c_lines.append("const uint16_t gMoveSfxStructCount = (uint16_t)(sizeof(gMoveSfxStructs)/sizeof(gMoveSfxStructs[0]));")
    c_lines.append("")

    return "\n".join(h_lines), "\n".join(c_lines)


def main() -> None:
    symbols = parse_move_symbols()
    header_map = parse_header_map()
    files = load_sfx_files()
    h_text, c_text = emit(symbols, header_map, files)
    OUT_H.write_text(h_text, encoding="ascii")
    OUT_C.write_text(c_text, encoding="ascii")
    print(f"Wrote {OUT_H} and {OUT_C} ({len(symbols)} move-linked symbols scanned)")


if __name__ == "__main__":
    main()
