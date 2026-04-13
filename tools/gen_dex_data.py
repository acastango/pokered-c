#!/usr/bin/env python3
"""gen_dex_data.py — extract Pokédex entries from pokered-master ASM.

Generates src/data/dex_data.h and src/data/dex_data.c with:
  - species category (e.g. "SEED")
  - height in feet + inches
  - weight in tenths of a pound
  - description text (ASCII, \\f = page break)

Index: gDexEntries[dex_number], dex_number 1-151. [0] unused.
"""

import re
import os

ROOT    = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
ASM     = os.path.join(ROOT, "pokered-master")
OUT_H   = os.path.join(ROOT, "src", "data", "dex_data.h")
OUT_C   = os.path.join(ROOT, "src", "data", "dex_data.c")

# ---- Pokemon name → dex number mapping (from pokemon_names_gen.h) ------
def load_pokemon_names():
    """Returns dict: uppercase_name → dex_number (1-151)."""
    path = os.path.join(ROOT, "src", "data", "pokemon_names_gen.h")
    name_to_dex = {}
    pat = re.compile(r'\[\s*(\d+)\s*\]\s*=\s*"([^"]+)"')
    with open(path) as f:
        for line in f:
            m = pat.search(line)
            if m:
                dex_num = int(m.group(1))
                name    = m.group(2).upper()
                name_to_dex[name] = dex_num
    return name_to_dex

# ---- Parse dex_entries.asm → dict: entry_label → (category, ft, inch, weight) --
def load_dex_entries():
    path = os.path.join(ASM, "data", "pokemon", "dex_entries.asm")
    entries = {}  # label (e.g. "Bulbasaur") → dict

    label_pat  = re.compile(r'^(\w+)DexEntry:')
    cat_pat    = re.compile(r'db\s+"([^@"]+)@?"')
    hw_pat     = re.compile(r'db\s+(\d+)\s*,\s*(\d+)')
    wt_pat     = re.compile(r'dw\s+(\d+)')

    current = None
    state   = None   # 'cat', 'hw', 'wt'

    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith(';'):
                continue

            m = label_pat.match(s)
            if m:
                current = m.group(1)
                entries[current] = {}
                state = 'cat'
                continue

            if current is None:
                continue

            if state == 'cat':
                m = cat_pat.search(s)
                if m:
                    entries[current]['category'] = m.group(1).strip()
                    state = 'hw'
            elif state == 'hw':
                m = hw_pat.search(s)
                if m:
                    entries[current]['ft']   = int(m.group(1))
                    entries[current]['inch'] = int(m.group(2))
                    state = 'wt'
            elif state == 'wt':
                m = wt_pat.search(s)
                if m:
                    entries[current]['weight'] = int(m.group(1))
                    state = None
                    current = None

    return entries

# ---- Parse dex_text.asm → dict: entry_label → description string --------
def load_dex_text():
    path = os.path.join(ASM, "data", "pokemon", "dex_text.asm")
    texts = {}  # label (e.g. "Bulbasaur") → str

    label_pat = re.compile(r'^_(\w+)DexEntry::')
    text_pat  = re.compile(r'text\s+"([^"]*)"')
    next_pat  = re.compile(r'next\s+"([^"]*)"')
    page_pat  = re.compile(r'page\s+"([^"]*)"')
    end_pat   = re.compile(r'\bdex\b')

    current = None
    pages   = []
    cur_pg  = []

    def flush(label, pages, cur_pg):
        if cur_pg:
            pages.append("\\n".join(cur_pg))
        if label:
            texts[label] = "\\f".join(pages)

    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith(';'):
                continue

            m = label_pat.match(s)
            if m:
                flush(current, pages, cur_pg)
                current = m.group(1)
                pages   = []
                cur_pg  = []
                continue

            if current is None:
                continue

            m = text_pat.search(s)
            if m:
                cur_pg = [m.group(1)]
                continue

            m = next_pat.search(s)
            if m:
                if not cur_pg:
                    cur_pg = [""]
                cur_pg.append(m.group(1))
                continue

            m = page_pat.search(s)
            if m:
                if cur_pg:
                    pages.append("\\n".join(cur_pg))
                cur_pg = [m.group(1)]
                continue

            if end_pat.search(s):
                flush(current, pages, cur_pg)
                current = None
                pages   = []
                cur_pg  = []

    flush(current, pages, cur_pg)
    return texts

# ---- Pokemon name → entry label mapping --------------------------------
# "BulbasaurDexEntry" → pokemon "BULBASAUR" dex 1
# We derive pokemon name by normalising the label: "Bulbasaur" → "BULBASAUR"
# Special cases: "NidoranF" → "NIDORAN♀", "NidoranM" → "NIDORAN♂"
LABEL_OVERRIDES = {
    "NidoranF":  "NIDORAN.F",
    "NidoranM":  "NIDORAN.M",
    "MrMime":    "MR.MIME",
    "Farfetchd": "FARFETCH'D",
}

def label_to_pokemon_name(label):
    if label in LABEL_OVERRIDES:
        return LABEL_OVERRIDES[label]
    # Insert space before each uppercase letter that follows a lower case,
    # then upper-case everything.
    # "Bulbasaur" → "BULBASAUR"
    return label.upper()

# ---- Assemble final table indexed by dex number -------------------------
def build_table():
    name_to_dex = load_pokemon_names()
    dex_entries  = load_dex_entries()
    dex_texts    = load_dex_text()

    table = [None] * 152  # table[0] unused

    for label, data in dex_entries.items():
        if 'category' not in data:
            continue
        pname = label_to_pokemon_name(label)
        # Try direct uppercase match first
        dex_num = name_to_dex.get(pname)
        if dex_num is None:
            # Fallback: try matching without spaces/punctuation
            pname_clean = re.sub(r'[^A-Z0-9]', '', pname)
            for k, v in name_to_dex.items():
                if re.sub(r'[^A-Z0-9]', '', k) == pname_clean:
                    dex_num = v
                    break
        if dex_num is None:
            print(f"  WARNING: could not map label '{label}' (pname='{pname}')")
            continue

        desc = dex_texts.get(label, "")
        table[dex_num] = {
            'label':    label,
            'category': data['category'],
            'ft':       data['ft'],
            'inch':     data['inch'],
            'weight':   data['weight'],
            'desc':     desc,
        }

    return table

# ---- C string escape ---------------------------------------------------
def c_escape(s):
    return s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')

# ---- Generate files ----------------------------------------------------
def generate():
    table = build_table()
    missing = [i for i in range(1, 152) if table[i] is None]
    if missing:
        print(f"  WARNING: missing entries for dex numbers: {missing}")

    # ------ dex_data.h ------
    h = """\
#pragma once
/* dex_data.h — Pokédex entry data for all 151 Pokémon.
 * Generated by tools/gen_dex_data.py from pokered-master ASM.
 * Index gDexEntries[dex_number], dex_number 1-151. [0] unused.
 */
#include <stdint.h>

typedef struct {
    const char *category;    /* species category, e.g. "SEED" */
    uint8_t     height_ft;
    uint8_t     height_in;
    uint16_t    weight;      /* tenths of a pound */
    const char *description; /* ASCII, \\\\f = page break */
} dex_entry_t;

#define NUM_DEX_ENTRIES 152
extern const dex_entry_t gDexEntries[NUM_DEX_ENTRIES];
"""
    with open(OUT_H, 'w') as f:
        f.write(h)
    print(f"  Wrote {OUT_H}")

    # ------ dex_data.c ------
    lines = []
    lines.append('/* dex_data.c — Pokédex entry data. Generated by tools/gen_dex_data.py */')
    lines.append('#include "dex_data.h"')
    lines.append('')
    lines.append('const dex_entry_t gDexEntries[NUM_DEX_ENTRIES] = {')
    lines.append('    [0] = { NULL, 0, 0, 0, NULL },  /* unused */')
    for i in range(1, 152):
        e = table[i]
        if e is None:
            lines.append(f'    [{i}] = {{ NULL, 0, 0, 0, NULL }},  /* MISSING */')
        else:
            cat  = c_escape(e['category'])
            desc = c_escape(e['desc'])
            lines.append(
                f'    [{i}] = {{ "{cat}", {e["ft"]}, {e["inch"]}, {e["weight"]},'
                f' "{desc}" }},  /* {e["label"]} */'
            )
    lines.append('};')
    lines.append('')

    with open(OUT_C, 'w') as f:
        f.write('\n'.join(lines))
    print(f"  Wrote {OUT_C}")

if __name__ == '__main__':
    print("Generating Pokédex data...")
    generate()
    print("Done.")
