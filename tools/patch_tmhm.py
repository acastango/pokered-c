#!/usr/bin/env python3
"""patch_tmhm.py — Injects .tmhm={...} fields into base_stats.c.

Runs gen_tmhm_data to get the data, then patches base_stats.c in-place.
Each line like:
    [N] = { ..., .growth_rate=X },  /* name */
becomes:
    [N] = { ..., .growth_rate=X, .tmhm={0xAB, ...} },  /* name */
"""
import os
import re
import sys

# Add tools dir to path so we can import gen_tmhm_data
sys.path.insert(0, os.path.dirname(__file__))
import gen_tmhm_data

BASE_STATS_C = os.path.join(os.path.dirname(__file__), '..', 'src', 'data', 'base_stats.c')

def main():
    # Generate tmhm data keyed by dex_id
    dex_map = gen_tmhm_data.parse_dex_constants(gen_tmhm_data.DEX_CONSTANTS)
    import glob
    asm_files = glob.glob(os.path.join(gen_tmhm_data.BASE_STATS_DIR, '*.asm'))
    tmhm_by_dex = {}
    for f in asm_files:
        r = gen_tmhm_data.parse_base_stats_file(f, dex_map)
        if r:
            dex_id, byte_vals, name = r
            tmhm_by_dex[dex_id] = byte_vals

    with open(BASE_STATS_C, encoding='utf-8') as f:
        content = f.read()

    lines = content.splitlines(keepends=True)
    out = []
    changed = 0

    for line in lines:
        # Match entries like: "    [N] = { ..., .growth_rate=X, [optional .tmhm=...] },  /* name */"
        m = re.match(r'(\s*\[(\d+)\]\s*=\s*\{)(.*?)(\},\s*/\*.*\*/\s*)$', line)
        if m:
            prefix   = m.group(1)
            dex_id   = int(m.group(2))
            body     = m.group(3)
            suffix   = m.group(4)

            if dex_id in tmhm_by_dex:
                # Remove existing .tmhm field if present
                body = re.sub(r',\s*\.tmhm\s*=\s*\{[^}]*\}', '', body)
                hex_bytes = ', '.join(f'0x{b:02X}' for b in tmhm_by_dex[dex_id])
                body = body.rstrip() + f', .tmhm={{{hex_bytes}}}'
                line = f'{prefix}{body}{suffix}'
                changed += 1

        out.append(line)

    with open(BASE_STATS_C, 'w', encoding='utf-8') as f:
        f.writelines(out)

    print(f'Patched {changed} entries in base_stats.c')

if __name__ == '__main__':
    main()
