#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_events.py -- Extract pokered event / wild-mon data into C source files.

Generates:
  src/data/event_data.h / .c  -- warp + NPC + sign event tables per map
  src/data/wild_data.h / .c   -- grass wild-mon tables per map
"""

import re, sys
from pathlib import Path

SRC = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT = Path(r"C:\Users\Anthony\pokered\src\data")

NUM_MAPS = 248


def wfile(name, content):
    path = OUT / name
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"  wrote {path}  ({len(content)} chars)")


def strip_comments(text):
    """Remove ASM ; comments, return cleaned text."""
    return re.sub(r";[^\n]*", "", text)


# ---------------------------------------------------------------------------
# 1. Map order: data/maps/map_header_pointers.asm -> list of CamelCase names
# ---------------------------------------------------------------------------

def parse_map_order(ptrs_asm):
    text = ptrs_asm.read_text(encoding="utf-8")
    text = strip_comments(text)
    names = []
    for m in re.finditer(r"dw\s+(\w+)_h\b", text):
        names.append(m.group(1))
    return names   # index == map_id


# ---------------------------------------------------------------------------
# 2. Map constants: constants/map_constants.asm -> dict NAME -> map_id
#    We read the hex comment $XX to get the ID directly.
# ---------------------------------------------------------------------------

def parse_map_ids(const_asm):
    """Returns dict: UPPER_SNAKE_NAME -> int map_id"""
    text = const_asm.read_text(encoding="utf-8")
    result = {}
    # Match lines like:  map_const PALLET_TOWN,   10,  9 ; $00
    for m in re.finditer(r"map_const\s+(\w+)\s*,.*?;\s*\$([0-9A-Fa-f]+)", text):
        result[m.group(1)] = int(m.group(2), 16)
    return result


# ---------------------------------------------------------------------------
# 3. Sprite constants: constants/sprite_constants.asm -> dict SPRITE_NAME -> id
# ---------------------------------------------------------------------------

def parse_sprite_ids(sprite_asm):
    """Returns dict: full_const_name (e.g. SPRITE_OAK) -> int id"""
    text = sprite_asm.read_text(encoding="utf-8")
    result = {}
    for m in re.finditer(r"const\s+(SPRITE_\w+)\s*;\s*\$([0-9A-Fa-f]+)", text):
        result[m.group(1)] = int(m.group(2), 16)
    return result


# ---------------------------------------------------------------------------
# 4. Pokemon constants: constants/pokemon_constants.asm -> dict NAME -> id
# ---------------------------------------------------------------------------

def parse_species_ids(pokemon_asm):
    """Returns dict: species_name -> int id"""
    text = pokemon_asm.read_text(encoding="utf-8")
    result = {}
    for m in re.finditer(r"const\s+(\w+)\s*;\s*\$([0-9A-Fa-f]+)", text):
        result[m.group(1)] = int(m.group(2), 16)
    return result


def parse_item_ids(item_asm):
    """Returns dict: ITEM_NAME -> int item_id (from ; $XX comments).
    Handles plain `const NAME ; $XX` entries and `add_tm/add_hm NAME ; $XX` entries.
    add_tm WATER_GUN ; $D4  ->  TM_WATER_GUN = 0xD4
    add_hm CUT       ; $C4  ->  HM_CUT       = 0xC4
    """
    text = item_asm.read_text(encoding="utf-8")
    result = {}
    # Plain items: `const NO_ITEM ; $00`
    for m in re.finditer(r"^\s*const\s+(\w+)\s*;\s*\$([0-9A-Fa-f]+)", text, re.M):
        result[m.group(1)] = int(m.group(2), 16)
    # TMs: `add_tm WATER_GUN ; $D4`  ->  TM_WATER_GUN = 0xD4
    for m in re.finditer(r"^\s*add_tm\s+(\w+)\s*;\s*\$([0-9A-Fa-f]+)", text, re.M):
        result[f"TM_{m.group(1)}"] = int(m.group(2), 16)
    # HMs: `add_hm CUT ; $C4`  ->  HM_CUT = 0xC4
    for m in re.finditer(r"^\s*add_hm\s+(\w+)\s*;\s*\$([0-9A-Fa-f]+)", text, re.M):
        result[f"HM_{m.group(1)}"] = int(m.group(2), 16)
    return result


# ---------------------------------------------------------------------------
# 5. Parse text/*.asm -> dict: _LabelName -> decoded C string
# ---------------------------------------------------------------------------

def _apply_text_substitutions(s):
    """Apply pokered text macro substitutions to a raw string value."""
    # Escape backslash and double-quote first so we don't double-escape
    s = s.replace("\\", "\\\\")
    s = s.replace('"', '\\"')
    # Pokered special tokens -> C-friendly placeholders
    s = s.replace("#MON", "POKEMON")
    s = s.replace("#", "POKE")
    s = s.replace("<PLAYER>", "{PLAYER}")
    s = s.replace("<RIVAL>", "{RIVAL}")
    return s


def decode_text_file(asm_path):
    """
    Parse a text/*.asm file and return dict: label_name -> decoded_string.

    Labels have the form  _SomeLabelName::  (double-colon, leading underscore).
    Body directives:
        text "..."    -- starts a new string (no leading separator)
        line "..."    -- newline before content
        cont "..."    -- newline before content (same as line in our output)
        para "..."    -- form-feed before content
        done          -- end of string (include in result)
        text_end      -- end of string
        prompt        -- end of string (wait for button)
        text_bcd ...  -- numeric display; skip content, end string
        text_decimal  -- skip
        text_ram ...  -- variable text; just close string
        text_start    -- ignore (used as a preamble in some files)
        db ...        -- ignore (raw bytes, not human-readable text)
    """
    raw = asm_path.read_text(encoding="utf-8")
    result = {}

    current_label = None
    current_parts = []   # list of (prefix, text_value)
    # prefix: "" for text, "\n" for line/cont, "\f" for para

    def flush():
        nonlocal current_label, current_parts
        if current_label is not None and current_parts:
            combined = "".join(p + v for p, v in current_parts)
            result[current_label] = combined
        current_label = None
        current_parts = []

    for line in raw.splitlines():
        # Detect label lines:  _LabelName:: or _LabelName:
        lm = re.match(r"^(_\w+)::", line)
        if lm:
            flush()
            current_label = lm.group(1)
            current_parts = []
            continue

        stripped = line.strip()

        # Skip blank lines and comment-only lines
        if not stripped or stripped.startswith(";"):
            continue
        # Strip inline comment
        stripped = re.sub(r"\s*;[^\n]*$", "", stripped).strip()
        if not stripped:
            continue

        if current_label is None:
            continue

        # Parse directive + optional quoted string
        dm = re.match(r'^(\w+)(?:\s+"(.*)")?', stripped)
        if not dm:
            continue

        directive = dm.group(1)
        value     = dm.group(2) if dm.group(2) is not None else ""

        if directive == "text":
            # Start of a new text block for this label
            # (some labels have multiple text blocks -- rare, but append)
            current_parts.append(("", _apply_text_substitutions(value)))

        elif directive in ("line", "cont"):
            current_parts.append(("\n", _apply_text_substitutions(value)))

        elif directive == "para":
            current_parts.append(("\f", _apply_text_substitutions(value)))

        elif directive in ("done", "text_end", "prompt"):
            # End of this text block; flush it
            flush()

        elif directive in ("text_bcd", "text_decimal", "text_ram", "text_start"):
            # Non-human-readable or variable content -- close string as-is
            flush()

        # db, text_asm, etc. inside a text label body -- ignore

    flush()
    return result


def parse_all_text_files(text_dir):
    """
    Parse all text/*.asm files.
    Returns dict: _LabelName -> decoded_string
    """
    result = {}
    for f in sorted(text_dir.glob("*.asm")):
        decoded = decode_text_file(f)
        result.update(decoded)
    return result


# ---------------------------------------------------------------------------
# 6. Parse scripts/*.asm -> per-map TextPointers + text_far resolution
#    Returns dict: map_camel_name -> (text_const_to_func, func_to_text_label)
#      text_const_to_func:  TEXT_XXX -> FunctionName
#      func_to_text_label:  FunctionName -> _ActualLabel
# ---------------------------------------------------------------------------

def _parse_one_script(asm_path):
    """
    Parse a single scripts/MapName.asm file.

    Returns (text_const_to_func, func_to_text_label):
      text_const_to_func:  {TEXT_XXX: "FuncName", ...}
      func_to_text_label:  {"FuncName": "_ActualLabel", ...}

    For text_asm functions (complex scripted text), we look for the FIRST
    text_far directive encountered anywhere inside the function body, including
    under local (.Label) sub-labels.
    """
    raw = asm_path.read_text(encoding="utf-8")

    # ---- Step 1: Build TextPointers table ----
    # Find the TextPointers block and parse dw_const entries.
    text_const_to_func = {}
    tp_match = re.search(
        r"(\w+_TextPointers:\s*\n(?:[ \t]+def_text_pointers[ \t]*\n)?)(.*?)(?=\n\w)",
        raw, re.DOTALL
    )
    if tp_match:
        tp_body = tp_match.group(2)
        for m in re.finditer(
            r"dw_const\s+(\w+)\s*,\s*(TEXT_\w+)", tp_body
        ):
            func_name  = m.group(1)
            text_const = m.group(2)
            text_const_to_func[text_const] = func_name

    # ---- Step 2: For each function, find the first text_far _Label ----
    # Strategy: split the file into top-level function blocks.
    # A top-level label is an identifier at column 0 followed by ':' (not '::').
    # Local labels (.something:) are part of the preceding top-level function.

    func_to_text_label = {}

    # Split file into blocks by top-level labels
    # A top-level label: starts at column 0, word chars, then ':'
    # but NOT '::' (those are exported labels, not function entry points here)
    # We also want to handle labels like "PalletTownGirlText:"
    blocks = re.split(r"\n(?=[A-Za-z]\w*:(?!:))", raw)

    for block in blocks:
        # Get the top-level label name for this block
        hdr = re.match(r"^([A-Za-z]\w*):", block)
        if not hdr:
            continue
        func_name = hdr.group(1)

        # Find the first text_far _Something in this block
        # (may be at top level or under a local label)
        tf_match = re.search(r"text_far\s+(_\w+)", block)
        if tf_match:
            func_to_text_label[func_name] = tf_match.group(1)

    return text_const_to_func, func_to_text_label


def parse_all_scripts(script_dir):
    """
    Parse all scripts/*.asm files.
    Returns dict: camel_name -> (text_const_to_func, func_to_text_label)
    """
    result = {}
    for f in sorted(script_dir.glob("*.asm")):
        camel_name = f.stem  # e.g. "PalletTown"
        tc2f, f2l = _parse_one_script(f)
        result[camel_name] = (tc2f, f2l)
    return result


# ---------------------------------------------------------------------------
# 7. Text resolution helper
#    Given a TEXT_CONST and the map's script data, resolve to a C string.
# ---------------------------------------------------------------------------

def resolve_text(text_const, script_data_for_map, all_texts):
    """
    Resolve a TEXT_XXX constant to its decoded C string.

    text_const:            e.g. "TEXT_PALLETTOWN_GIRL"
    script_data_for_map:   (text_const_to_func, func_to_text_label) or None
    all_texts:             dict: _LabelName -> decoded string

    Returns decoded string, or None if not found at any step.
    """
    if not text_const or script_data_for_map is None:
        return None

    text_const_to_func, func_to_text_label = script_data_for_map

    # Step 1: TEXT_XXX -> function name
    func_name = text_const_to_func.get(text_const)
    if not func_name:
        return None

    # Step 2: function name -> _ActualLabel
    actual_label = func_to_text_label.get(func_name)
    if not actual_label:
        return None

    # Step 3: _ActualLabel -> decoded string
    return all_texts.get(actual_label)


# ---------------------------------------------------------------------------
# 8. Parse one map object file -> (border, warps, npcs, signs)
#    warps: list of (x, y, dest_map_name, dest_warp_id_1indexed)
#    npcs:  list of (x, y, sprite_const, movement_str, text_const)
#    signs: list of (x, y, text_const)
# ---------------------------------------------------------------------------

def parse_object_file(obj_path):
    text = obj_path.read_text(encoding="utf-8")
    text = strip_comments(text)

    # border block
    border = 0
    mb = re.search(r"db\s+\$([0-9A-Fa-f]+)", text)
    if mb:
        border = int(mb.group(1), 16)

    # warp_event X, Y, DEST_MAP, DEST_WARP_ID
    warps = []
    for m in re.finditer(
        r"warp_event\s+(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*,\s*(\d+)", text
    ):
        warps.append((int(m.group(1)), int(m.group(2)), m.group(3), int(m.group(4))))

    # bg_event X, Y, TEXT_LABEL
    signs = []
    for m in re.finditer(
        r"bg_event\s+(\d+)\s*,\s*(\d+)\s*,\s*(TEXT_\w+)", text
    ):
        signs.append((int(m.group(1)), int(m.group(2)), m.group(3)))

    # Item balls: SPRITE_POKE_BALL with 7th field = item constant
    # Format: object_event X, Y, SPRITE_POKE_BALL, STAY, NONE, TEXT_XXX, ITEM_CONST
    items = []
    item_ball_coords = set()
    for m in re.finditer(
        r"object_event\s+(\d+)\s*,\s*(\d+)\s*,\s*SPRITE_POKE_BALL\s*,"
        r"\s*\w+\s*,\s*\w+\s*,\s*TEXT_\w+\s*,\s*(\w+)",
        text
    ):
        ax, ay = int(m.group(1)), int(m.group(2))
        item_ball_coords.add((ax, ay))
        items.append((ax, ay, m.group(3)))  # (x, y, item_const)

    # object_event X, Y, SPRITE_CONST, MOVEMENT, DIRECTION, TEXT_LABEL[, ...]
    # Fields: 0=X, 1=Y, 2=SPRITE, 3=MOVEMENT, 4=DIRECTION, 5=TEXT_LABEL
    # Skip SPRITE_POKE_BALL — those are item balls handled above.
    npcs = []
    for m in re.finditer(
        r"object_event\s+(\d+)\s*,\s*(\d+)\s*,\s*(SPRITE_\w+)\s*,\s*(\w+)\s*,"
        r"\s*(\w+)\s*,\s*(TEXT_\w+)",
        text
    ):
        if m.group(3) == "SPRITE_POKE_BALL":
            continue
        npcs.append((
            int(m.group(1)),   # x
            int(m.group(2)),   # y
            m.group(3),        # sprite_const
            m.group(4),        # movement
            m.group(6),        # text_const (field 6, after DIRECTION)
        ))

    return border, warps, npcs, items, signs


# ---------------------------------------------------------------------------
# 9. Parse wild data
# ---------------------------------------------------------------------------

def parse_wild_data(grass_water_asm, src_root):
    """
    Returns dict: label -> (grass_rate, [(level, species_name), ...])
    """
    result = {}

    def parse_one(text, label):
        mg = re.search(r"def_grass_wildmons\s+(\d+)(.*?)end_grass_wildmons",
                       text, re.DOTALL)
        if not mg:
            return
        rate = int(mg.group(1))
        slots = []
        if rate > 0:
            for ms in re.finditer(r"db\s+(\d+)\s*,\s*(\w+)", mg.group(2)):
                slots.append((int(ms.group(1)), ms.group(2)))
        result[label] = (rate, slots)

    main_text = grass_water_asm.read_text(encoding="utf-8")
    includes = re.findall(r'INCLUDE\s+"([^"]+)"', main_text)

    for inc in includes:
        inc_path = src_root / inc
        if not inc_path.exists():
            print(f"  WARNING: missing wild include {inc_path}", file=sys.stderr)
            continue
        inc_text = strip_comments(inc_path.read_text(encoding="utf-8"))
        lm = re.match(r"\s*(\w+):", inc_text)
        if lm:
            label = lm.group(1)
            parse_one(inc_text, label)

    return result


# ---------------------------------------------------------------------------
# 10. Parse WildDataPointers table -> list of label names (index == map_id)
# ---------------------------------------------------------------------------

def parse_wild_pointer_table(grass_water_asm):
    """Returns list[248] of label names."""
    text = grass_water_asm.read_text(encoding="utf-8")
    labels = []
    in_table = False
    for line in text.splitlines():
        stripped = line.strip()
        if "WildDataPointers:" in stripped:
            in_table = True
            continue
        if in_table:
            if re.match(r"assert_table_length|dw\s+-1", stripped):
                break
            m = re.match(r"dw\s+(\w+)", stripped)
            if m:
                labels.append(m.group(1))
            elif stripped and not stripped.startswith(";") and not stripped.startswith("table_width"):
                break
    return labels


# ---------------------------------------------------------------------------
# C string emission helper
# ---------------------------------------------------------------------------

def c_string_literal(s):
    """
    Wrap a decoded text string in a C string literal.
    The string is already escaped (backslash, double-quote) by the decoder.
    We only need to handle real newlines in the source (there shouldn't be any,
    but just in case), and emit the literal.
    """
    # Replace any actual newline characters with \n (shouldn't happen but safe)
    s = s.replace("\r", "").replace("\n", "\\n").replace("\f", "\\f")
    return f'"{s}"'


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

print("Parsing map order...")
map_order = parse_map_order(SRC / "data/maps/map_header_pointers.asm")
print(f"  {len(map_order)} maps in pointer table")

print("Parsing map IDs...")
map_ids = parse_map_ids(SRC / "constants/map_constants.asm")
print(f"  {len(map_ids)} map constants")

print("Parsing sprite constants...")
sprite_ids = parse_sprite_ids(SRC / "constants/sprite_constants.asm")
print(f"  {len(sprite_ids)} sprite constants")

print("Parsing pokemon constants...")
species_ids = parse_species_ids(SRC / "constants/pokemon_constants.asm")
print(f"  {len(species_ids)} species constants")

print("Parsing item constants...")
item_ids = parse_item_ids(SRC / "constants/item_constants.asm")
print(f"  {len(item_ids)} item constants")

print("Parsing wild data...")
wild_data = parse_wild_data(SRC / "data/wild/grass_water.asm", SRC)
print(f"  {len(wild_data)} wild mon label blocks parsed")

print("Parsing wild pointer table...")
wild_ptrs = parse_wild_pointer_table(SRC / "data/wild/grass_water.asm")
print(f"  {len(wild_ptrs)} entries in WildDataPointers")

print("Parsing text files...")
all_texts = parse_all_text_files(SRC / "text")
print(f"  {len(all_texts)} text labels decoded")

print("Parsing script files...")
all_scripts = parse_all_scripts(SRC / "scripts")
print(f"  {len(all_scripts)} script files parsed")

# Build a lookup: CamelCase map name -> map_id
camel_to_id = {name: i for i, name in enumerate(map_order)}

# ---------------------------------------------------------------------------
# Parse all map object files
# ---------------------------------------------------------------------------

print("Parsing map object files...")
obj_dir = SRC / "data/maps/objects"

# map_id -> (border, warps, npcs, signs) or absent if no file
map_events = {}

for map_id, camel_name in enumerate(map_order):
    obj_path = obj_dir / f"{camel_name}.asm"
    if obj_path.exists():
        border, warps, npcs, items, signs = parse_object_file(obj_path)
        map_events[map_id] = (border, warps, npcs, items, signs)

print(f"  {len(map_events)} maps have object files")

# ---------------------------------------------------------------------------
# Generate event_data.h
# ---------------------------------------------------------------------------

print("Generating event_data.h...")
event_h = """\
#pragma once
/* event_data.h -- Generated from pokered-master map object files. */
#include <stdint.h>

typedef struct {
    uint16_t x, y;          /* tile coords: x = asm_x*2, y = asm_y*2+1 */
    uint8_t  dest_map;
    uint8_t  dest_warp_idx; /* 0-indexed */
} map_warp_t;

typedef struct {
    uint16_t    x, y;       /* tile coords: x = asm_x*2, y = asm_y*2+1 */
    uint8_t     sprite_id;  /* SPRITE_* numeric value */
    uint8_t     movement;   /* 0=STAY, 1=WALK */
    const char *text;       /* decoded dialogue, or NULL */
} npc_event_t;

typedef struct {
    uint16_t    x, y;       /* tile coords: x = asm_x*2, y = asm_y*2+1 */
    const char *text;       /* decoded sign text, or NULL */
} sign_event_t;

typedef struct {
    uint16_t x, y;          /* tile coords: x = asm_x*2, y = asm_y*2+1 */
    uint8_t  item_id;       /* item ID (from item_constants.asm) */
} item_event_t;

typedef struct {
    const map_warp_t  *warps;
    uint8_t            num_warps;
    const npc_event_t *npcs;
    uint8_t            num_npcs;
    const sign_event_t *signs;
    uint8_t             num_signs;
    const item_event_t *items;
    uint8_t             num_items;
    uint8_t            border_block;
} map_events_t;

#define NUM_MAPS 248
extern const map_events_t gMapEvents[NUM_MAPS];
"""
wfile("event_data.h", event_h)

# ---------------------------------------------------------------------------
# Generate event_data.c
# ---------------------------------------------------------------------------

def clamp8(v):
    """Clamp an integer to [0, 65535] for safe uint16_t tile coord emission."""
    return min(65535, max(0, v))


print("Generating event_data.c...")
lines = [
    "/* event_data.c -- Generated from pokered-master map object files. */",
    '#include "event_data.h"',
    "",
]

# Tracking arrays emitted
has_warps   = {}
has_npcs    = {}
has_signs   = {}
has_items   = {}
warp_sym    = {}
npc_sym     = {}
sign_sym    = {}
item_sym    = {}
item_count  = {}

emitted_warp_arrays = {}
emitted_npc_arrays  = {}
emitted_sign_arrays = {}
emitted_item_arrays = {}

for map_id in range(NUM_MAPS):
    if map_id not in map_events:
        has_warps[map_id] = False
        has_npcs[map_id]  = False
        has_signs[map_id] = False
        has_items[map_id] = False
        warp_sym[map_id]  = "NULL"
        npc_sym[map_id]   = "NULL"
        sign_sym[map_id]  = "NULL"
        item_sym[map_id]  = "NULL"
        item_count[map_id] = 0
        continue

    border, warps, npcs, items, signs = map_events[map_id]
    camel_name = map_order[map_id] if map_id < len(map_order) else f"Map{map_id:03d}"

    # Get script data for this map (for text resolution)
    script_data = all_scripts.get(camel_name)

    # --- warp array ---
    if warps:
        if camel_name in emitted_warp_arrays:
            sym = f"kWarps_{camel_name}"
        else:
            sym = f"kWarps_{camel_name}"
            emitted_warp_arrays[camel_name] = map_id
            lines.append(f"static const map_warp_t {sym}[] = {{")
            for (ax, ay, dest_name, dest_warp1) in warps:
                tx = clamp8(ax * 2)
                ty = clamp8(ay * 2 + 1)
                dest_id  = map_ids.get(dest_name, 0xFF)
                dest_idx = dest_warp1 - 1
                lines.append(
                    f"    {{ {tx:3d}, {ty:3d}, {dest_id:#04x}, {dest_idx} }},  /* {dest_name} */"
                )
            lines.append("};")
            lines.append("")
        has_warps[map_id] = True
        warp_sym[map_id]  = sym
    else:
        has_warps[map_id] = False
        warp_sym[map_id]  = "NULL"

    # --- NPC array ---
    if npcs:
        if camel_name in emitted_npc_arrays:
            sym = f"kNpcs_{camel_name}"
        else:
            sym = f"kNpcs_{camel_name}"
            emitted_npc_arrays[camel_name] = map_id
            lines.append(f"static const npc_event_t {sym}[] = {{")
            for (ax, ay, sprite_const, movement, text_const) in npcs:
                tx    = clamp8(ax * 2)
                ty    = clamp8(ay * 2 + 1)
                sp_id = sprite_ids.get(sprite_const, 0)
                mov   = 1 if movement == "WALK" else 0
                text_str = resolve_text(text_const, script_data, all_texts)
                if text_str is not None:
                    text_field = c_string_literal(text_str)
                else:
                    text_field = "NULL"
                lines.append(
                    f"    {{ {tx:3d}, {ty:3d}, {sp_id:#04x}, {mov}, {text_field} }},"
                    f"  /* {sprite_const}, {movement}, {text_const} */"
                )
            lines.append("};")
            lines.append("")
        has_npcs[map_id] = True
        npc_sym[map_id]  = sym
    else:
        has_npcs[map_id] = False
        npc_sym[map_id]  = "NULL"

    # --- Sign array ---
    if signs:
        if camel_name in emitted_sign_arrays:
            sym = f"kSigns_{camel_name}"
        else:
            sym = f"kSigns_{camel_name}"
            emitted_sign_arrays[camel_name] = map_id
            lines.append(f"static const sign_event_t {sym}[] = {{")
            for (ax, ay, text_const) in signs:
                tx = clamp8(ax * 2)
                ty = clamp8(ay * 2 + 1)
                text_str = resolve_text(text_const, script_data, all_texts)
                if text_str is not None:
                    text_field = c_string_literal(text_str)
                else:
                    text_field = "NULL"
                lines.append(
                    f"    {{ {tx:3d}, {ty:3d}, {text_field} }},"
                    f"  /* {text_const} */"
                )
            lines.append("};")
            lines.append("")
        has_signs[map_id] = True
        sign_sym[map_id]  = sym
    else:
        has_signs[map_id] = False
        sign_sym[map_id]  = "NULL"

    # --- Item ball array ---
    if items:
        if camel_name in emitted_item_arrays:
            sym = f"kItems_{camel_name}"
        else:
            sym = f"kItems_{camel_name}"
            emitted_item_arrays[camel_name] = map_id
            lines.append(f"static const item_event_t {sym}[] = {{")
            for (ax, ay, item_const) in items:
                tx  = clamp8(ax * 2)
                ty  = clamp8(ay * 2 + 1)
                iid = item_ids.get(item_const, 0)
                lines.append(
                    f"    {{ {tx:3d}, {ty:3d}, {iid:#04x} }},"
                    f"  /* {item_const} */"
                )
            lines.append("};")
            lines.append("")
        has_items[map_id]  = True
        item_sym[map_id]   = sym
        item_count[map_id] = len(items)
    else:
        has_items[map_id]  = False
        item_sym[map_id]   = "NULL"
        item_count[map_id] = 0

# --- gMapEvents table ---
lines.append("const map_events_t gMapEvents[NUM_MAPS] = {")
for map_id in range(NUM_MAPS):
    if map_id not in map_events:
        lines.append(f"    [{map_id:#04x}] = {{ NULL, 0, NULL, 0, NULL, 0, NULL, 0, 0 }},")
        continue

    border, warps, npcs, items, signs = map_events[map_id]
    camel_name = map_order[map_id] if map_id < len(map_order) else f"Map{map_id:03d}"

    warp_ref  = warp_sym[map_id]
    nwarp     = len(warps)
    npc_ref   = npc_sym[map_id]
    nnpc      = len(npcs)
    sign_ref  = sign_sym[map_id]
    nsign     = len(signs)
    item_ref  = item_sym[map_id]
    nitem     = item_count[map_id]

    lines.append(
        f"    [{map_id:#04x}] = {{"
        f" {warp_ref}, {nwarp},"
        f" {npc_ref}, {nnpc},"
        f" {sign_ref}, {nsign},"
        f" {item_ref}, {nitem},"
        f" {border:#04x} }},"
    )
lines.append("};")
lines.append("")

wfile("event_data.c", "\n".join(lines))

# ---------------------------------------------------------------------------
# Generate wild_data.h
# ---------------------------------------------------------------------------

print("Generating wild_data.h...")
wild_h = """\
#pragma once
/* wild_data.h -- Generated from pokered-master wild pokemon data. */
#include <stdint.h>

typedef struct {
    uint8_t level;
    uint8_t species;  /* internal species ID */
} wild_slot_t;

typedef struct {
    uint8_t      rate;      /* encounter rate (0 = no encounters) */
    wild_slot_t  slots[10];
} wild_mons_t;

#define NUM_MAPS 248
extern const wild_mons_t gWildGrass[NUM_MAPS];
"""
wfile("wild_data.h", wild_h)

# ---------------------------------------------------------------------------
# Generate wild_data.c
# ---------------------------------------------------------------------------

print("Generating wild_data.c...")
wlines = [
    "/* wild_data.c -- Generated from pokered-master wild pokemon data. */",
    '#include "wild_data.h"',
    "",
    "const wild_mons_t gWildGrass[NUM_MAPS] = {",
]

# Pad wild_ptrs to NUM_MAPS if shorter
while len(wild_ptrs) < NUM_MAPS:
    wild_ptrs.append("NothingWildMons")

nothing_rate, _ = wild_data.get("NothingWildMons", (0, []))

for map_id in range(NUM_MAPS):
    label = wild_ptrs[map_id]
    entry = wild_data.get(label)
    if entry is None:
        wlines.append(
            f"    [{map_id:#04x}] = {{ 0, {{ {{0,0}},{{0,0}},{{0,0}},{{0,0}},{{0,0}},"
            f"{{0,0}},{{0,0}},{{0,0}},{{0,0}},{{0,0}} }} }},"
            f"  /* {label} (missing) */"
        )
        continue

    rate, slots = entry
    padded = slots[:10]
    while len(padded) < 10:
        padded.append((0, "NO_MON"))

    slot_strs = []
    for (lvl, sp_name) in padded:
        sp_id = species_ids.get(sp_name, 0)
        slot_strs.append(f"{{{lvl},{sp_id:#04x}}}")

    slots_joined = ", ".join(slot_strs)
    camel_name = map_order[map_id] if map_id < len(map_order) else f"Map{map_id:02X}"
    wlines.append(
        f"    [{map_id:#04x}] = {{ {rate}, {{ {slots_joined} }} }},"
        f"  /* {camel_name}: {label} */"
    )

wlines.append("};")
wlines.append("")

wfile("wild_data.c", "\n".join(wlines))

print("Done.")
