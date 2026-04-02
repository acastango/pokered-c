#!/usr/bin/env python3
"""Transform doubled coordinates to 1:1 ASM coordinates in event_data.c and connection_data.c.

Rules for event_data.c:
  - x values: currently asm_x * 2 (even). Divide by 2.
  - y values: currently asm_y * 2 + 1 (odd). Divide by 2 (integer division).
  - Only the FIRST TWO numeric fields in each struct initializer are x,y.

Rules for connection_data.c:
  - Format: .direction={dest_map, player_coord, adjust}
  - Do NOT change dest_map (first field, hex like 0x0C).
  - Divide player_coord by 2 (integer division).
  - Divide adjust by 2 (integer division). Handles negatives.
"""

import re
import os

EVENT_DATA = r"C:\Users\Anthony\pokered\src\data\event_data.c"
CONN_DATA  = r"C:\Users\Anthony\pokered\src\data\connection_data.c"


def transform_event_data(filepath):
    """Transform x,y coordinates in struct initializers in event_data.c."""
    with open(filepath, 'r') as f:
        content = f.read()

    changes = 0

    # Match lines inside struct initializer arrays: { x, y, ... }
    # We need to find lines that start with { and have at least two numeric fields.
    # The struct types are: map_warp_t, npc_event_t, sign_event_t, item_event_t, hidden_event_t
    #
    # Pattern: { <whitespace> <number>, <whitespace> <number>, <rest> }
    # Numbers can be decimal or hex (0x...).
    # We only want to transform the first two decimal numbers (x and y).

    def replace_xy(match):
        nonlocal changes
        prefix = match.group(1)   # everything before first number including {
        x_str = match.group(2)    # x value (decimal integer)
        mid = match.group(3)      # comma and whitespace between x and y
        y_str = match.group(4)    # y value (decimal integer)
        suffix = match.group(5)   # rest of line

        x_val = int(x_str)
        y_val = int(y_str)

        new_x = x_val // 2
        new_y = y_val // 2

        if new_x != x_val or new_y != y_val:
            changes += 1

        # Preserve field width formatting
        # Measure original width of x field
        x_width = len(x_str)
        y_width = len(y_str)

        new_x_str = f"{new_x:>{x_width}}"
        new_y_str = f"{new_y:>{y_width}}"

        return f"{prefix}{new_x_str}{mid}{new_y_str}{suffix}"

    # Pattern explanation:
    # (^\s*\{\s*) - line start, whitespace, opening brace, whitespace
    # (\d+)       - first decimal number (x)
    # (,\s*)      - comma + whitespace
    # (\d+)       - second decimal number (y)
    # (,\s*.*)    - rest of line
    pattern = r'(^\s*\{\s*)(\d+)(,\s*)(\d+)(,\s*.*)'

    new_content = re.sub(pattern, replace_xy, content, flags=re.MULTILINE)

    with open(filepath, 'w') as f:
        f.write(new_content)

    return changes


def transform_connection_data(filepath):
    """Transform player_coord and adjust in connection_data.c.

    Format: .north={0x0C, 71, 0}
    - Field 1 (dest_map): hex, do NOT change
    - Field 2 (player_coord): divide by 2
    - Field 3 (adjust): divide by 2 (can be negative)
    """
    with open(filepath, 'r') as f:
        content = f.read()

    changes = 0

    def replace_connection(match):
        nonlocal changes
        direction = match.group(1)    # .north={
        dest_map = match.group(2)     # 0x0C
        sep1 = match.group(3)         # ,
        coord_str = match.group(4)    # 71 or  71
        sep2 = match.group(5)         # ,
        adjust_str = match.group(6)   # 0 or   -20
        close = match.group(7)        # }

        coord_val = int(coord_str.strip())
        adjust_val = int(adjust_str.strip())

        # Integer division toward zero for negatives
        def idiv2(v):
            if v < 0:
                return -((-v) // 2)
            return v // 2

        new_coord = idiv2(coord_val)
        new_adjust = idiv2(adjust_val)

        if new_coord != coord_val or new_adjust != adjust_val:
            changes += 1

        # Preserve formatting width
        coord_stripped = coord_str.lstrip()
        coord_leading = coord_str[:len(coord_str) - len(coord_stripped)]
        coord_width = len(coord_stripped)

        adjust_stripped = adjust_str.lstrip()
        adjust_leading = adjust_str[:len(adjust_str) - len(adjust_stripped)]
        adjust_width = len(adjust_stripped)

        new_coord_s = f"{coord_leading}{new_coord:>{coord_width}}"
        new_adjust_s = f"{adjust_leading}{new_adjust:>{adjust_width}}"

        return f"{direction}{dest_map}{sep1}{new_coord_s}{sep2}{new_adjust_s}{close}"

    # Pattern: .direction={hex, number, number}
    # e.g. .north={0x0C,  71,    0}
    pattern = r'(\.\w+=\{)(0x[0-9A-Fa-f]+)(,)(\s*-?\d+)(,)(\s*-?\d+)(\})'

    new_content = re.sub(pattern, replace_connection, content)

    with open(filepath, 'w') as f:
        f.write(new_content)

    return changes


if __name__ == '__main__':
    print("=== Coordinate Transform: doubled -> 1:1 ASM ===\n")

    print(f"Processing {EVENT_DATA}...")
    event_changes = transform_event_data(EVENT_DATA)
    print(f"  {event_changes} lines with coordinate changes\n")

    print(f"Processing {CONN_DATA}...")
    conn_changes = transform_connection_data(CONN_DATA)
    print(f"  {conn_changes} connection entries with coordinate changes\n")

    print(f"Total: {event_changes + conn_changes} transformations applied.")

    # Verify: print first few entries from each file
    print("\n=== Verification: event_data.c (first 30 lines) ===")
    with open(EVENT_DATA, 'r') as f:
        for i, line in enumerate(f):
            if i >= 30:
                break
            print(line, end='')

    print("\n\n=== Verification: connection_data.c (first 15 lines) ===")
    with open(CONN_DATA, 'r') as f:
        for i, line in enumerate(f):
            if i >= 15:
                break
            print(line, end='')
