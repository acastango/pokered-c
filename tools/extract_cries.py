#!/usr/bin/env python3
"""extract_cries.py — Parse pokered-master cry ASM files and emit C cry data.

Reads:
  audio/sfx/cry00_1.asm .. cry25_1.asm  (38 base cries, _1 file has all 3 channels)
  data/pokemon/cries.asm                 (per-pokemon cry lookup table)

Writes:
  src/data/cry_data.h
  src/data/cry_data.c

Each cry_XX_1.asm contains three labelled sections:
  SFX_CryXX_1_Ch5  — square channel 1 (maps to audio ch[0])
  SFX_CryXX_1_Ch6  — square channel 2 (maps to audio ch[1])
  SFX_CryXX_1_Ch8  — noise channel    (maps to audio ch[3])

Macros parsed:
  duty_cycle_pattern d0, d1, d2, d3  — rotating 4-step duty; pattern byte = (d0<<6)|(d1<<4)|(d2<<2)|d3
  duty_cycle d                       — fixed duty; pattern byte = (d<<6)|(d<<4)|(d<<2)|d; rotate=0
  square_note len, vol, fade, freq   — one square note
  noise_note  len, vol, fade, nr43   — one noise note
  sound_ret                          — end of section

cries.asm format:  mon_cry SFX_CRY_XX, $pitch, $tempo
  base_cry_id = hex value of XX (e.g. SFX_CRY_1A → 0x1A)
  pitch_mod   = signed int8 applied to each note's 11-bit GB freq register
  tempo_mod   = wTempoModifier; sfx_tempo = tempo_mod + 0x80 (16-bit); duration = len*sfx_tempo>>8
"""

import re
import sys
import os

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
POKERED = r"C:/Users/Anthony/pokered/pokered-master"
SFX_DIR = os.path.join(POKERED, "audio/sfx")
CRIES_ASM = os.path.join(POKERED, "data/pokemon/cries.asm")
OUT_H = r"C:/Users/Anthony/pokered/src/data/cry_data.h"
OUT_C = r"C:/Users/Anthony/pokered/src/data/cry_data.c"

NUM_BASE_CRIES = 38  # cry00 .. cry25 (0x00 .. 0x25)

# ---------------------------------------------------------------------------
# Parse a single cry_XX_1.asm file
# Returns:  { 'ch5': {duty_pattern, rotate_duty, notes:[(len,vol,fade,freq),...]},
#             'ch6': {duty_pattern, rotate_duty, notes:[...]},
#             'ch8': {notes:[(len,vol,fade,nr43),...]} }
# ---------------------------------------------------------------------------
def parse_cry_file(path):
    result = {
        'ch5': {'duty_pattern': 0xC0, 'rotate_duty': 0, 'notes': []},
        'ch6': {'duty_pattern': 0xC0, 'rotate_duty': 0, 'notes': []},
        'ch8': {'notes': []},
    }

    current_ch = None

    with open(path, encoding='utf-8') as f:
        for raw_line in f:
            line = raw_line.strip()
            # Remove comments
            line = re.sub(r';.*', '', line).strip()
            if not line:
                continue

            # Section labels
            if re.search(r'_Ch5\s*:', line):
                current_ch = 'ch5'
                continue
            if re.search(r'_Ch6\s*:', line):
                current_ch = 'ch6'
                continue
            if re.search(r'_Ch8\s*:', line):
                current_ch = 'ch8'
                continue

            if current_ch is None:
                continue

            # sound_ret — end of this channel's data
            if re.match(r'sound_ret\b', line):
                current_ch = None
                continue

            # duty_cycle_pattern d0, d1, d2, d3
            m = re.match(r'duty_cycle_pattern\s+(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)', line)
            if m and current_ch in ('ch5', 'ch6'):
                d = [int(m.group(i)) for i in range(1, 5)]
                pattern = (d[0] << 6) | (d[1] << 4) | (d[2] << 2) | d[3]
                result[current_ch]['duty_pattern'] = pattern
                result[current_ch]['rotate_duty'] = 1
                continue

            # duty_cycle d
            m = re.match(r'duty_cycle\s+(\d+)', line)
            if m and current_ch in ('ch5', 'ch6'):
                d = int(m.group(1)) & 3
                pattern = (d << 6) | (d << 4) | (d << 2) | d
                result[current_ch]['duty_pattern'] = pattern
                result[current_ch]['rotate_duty'] = 0
                continue

            # square_note len, vol, fade, freq
            m = re.match(r'square_note\s+(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)', line)
            if m and current_ch in ('ch5', 'ch6'):
                note = (int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)))
                result[current_ch]['notes'].append(note)
                continue

            # noise_note len, vol, fade, nr43
            m = re.match(r'noise_note\s+(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)', line)
            if m and current_ch == 'ch8':
                note = (int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)))
                result['ch8']['notes'].append(note)
                continue

    return result


# ---------------------------------------------------------------------------
# Parse data/pokemon/cries.asm
# Returns list of 191 tuples: (base_cry_id, pitch_mod_byte, tempo_mod_byte)
# ---------------------------------------------------------------------------
def parse_pokemon_cries(path):
    entries = []
    with open(path, encoding='utf-8') as f:
        for line in f:
            line = re.sub(r';.*', '', line).strip()
            # mon_cry SFX_CRY_XX, $pp, $tt
            m = re.match(r'mon_cry\s+SFX_CRY_([0-9A-Fa-f]+)\s*,\s*\$([0-9A-Fa-f]+)\s*,\s*\$([0-9A-Fa-f]+)', line)
            if m:
                base = int(m.group(1), 16)
                pitch = int(m.group(2), 16)
                tempo = int(m.group(3), 16)
                entries.append((base, pitch, tempo))
    return entries


# ---------------------------------------------------------------------------
# Emit C
# ---------------------------------------------------------------------------
def emit_h(cries, pokemon_cries):
    lines = []
    lines.append('/* cry_data.h — Pokémon cry note data extracted from pokered-master.')
    lines.append(' * Generated by tools/extract_cries.py — DO NOT EDIT.')
    lines.append(' *')
    lines.append(' * ALWAYS refer to pokered-master audio/sfx/cry*.asm before modifying.')
    lines.append(' */')
    lines.append('#pragma once')
    lines.append('#include <stdint.h>')
    lines.append('')
    lines.append('/* One square (CH1/CH2) note */')
    lines.append('typedef struct {')
    lines.append('    uint8_t  len;    /* note length (1-15 frames base) */')
    lines.append('    uint8_t  vol;    /* initial volume 0-15 */')
    lines.append('    uint8_t  fade;   /* envelope decrease pace (0=off) */')
    lines.append('    uint16_t freq;   /* 11-bit GB freq register value */')
    lines.append('} cry_sq_note_t;')
    lines.append('')
    lines.append('/* One noise (CH4) note */')
    lines.append('typedef struct {')
    lines.append('    uint8_t len;   /* note length */')
    lines.append('    uint8_t vol;')
    lines.append('    uint8_t fade;')
    lines.append('    uint8_t nr43;  /* NR43: shift<<4 | width_bit<<3 | divisor */')
    lines.append('} cry_noise_note_t;')
    lines.append('')
    lines.append('/* Square channel descriptor */')
    lines.append('typedef struct {')
    lines.append('    uint8_t              duty_pattern; /* (d0<<6)|(d1<<4)|(d2<<2)|d3 */')
    lines.append('    uint8_t              rotate_duty;  /* 1=duty_cycle_pattern, 0=fixed */')
    lines.append('    uint8_t              n_notes;')
    lines.append('    const cry_sq_note_t *notes;')
    lines.append('} cry_sq_ch_t;')
    lines.append('')
    lines.append('/* Noise channel descriptor */')
    lines.append('typedef struct {')
    lines.append('    uint8_t                  n_notes;')
    lines.append('    const cry_noise_note_t  *notes;')
    lines.append('} cry_noise_ch_t;')
    lines.append('')
    lines.append('/* Full 3-channel cry definition */')
    lines.append('typedef struct {')
    lines.append('    cry_sq_ch_t    ch5;  /* square CH1 → audio ch[0] */')
    lines.append('    cry_sq_ch_t    ch6;  /* square CH2 → audio ch[1] */')
    lines.append('    cry_noise_ch_t ch8;  /* noise  CH4 → audio ch[3] */')
    lines.append('} cry_def_t;')
    lines.append('')
    lines.append('/* Per-Pokémon cry parameters (internal species 1..191, table is 0-based) */')
    lines.append('typedef struct {')
    lines.append('    uint8_t base_cry;  /* index into g_cry_defs[0..37] */')
    lines.append('    int8_t  pitch_mod; /* wFrequencyModifier: added to each note freq */')
    lines.append('    uint8_t tempo_mod; /* wTempoModifier: sfx_tempo = tempo_mod + 0x80 */')
    lines.append('} pokemon_cry_t;')
    lines.append('')
    lines.append(f'#define NUM_BASE_CRIES      {NUM_BASE_CRIES}')
    lines.append(f'#define NUM_POKEMON_CRIES   {len(pokemon_cries)}')
    lines.append('')
    lines.append('/* Base cry definitions indexed 0..37 (cry00..cry25 hex) */')
    lines.append('extern const cry_def_t     g_cry_defs[NUM_BASE_CRIES];')
    lines.append('')
    lines.append('/* Per-species lookup: index = (internal_species_id - 1) */')
    lines.append('extern const pokemon_cry_t g_pokemon_cries[NUM_POKEMON_CRIES];')
    lines.append('')
    return '\n'.join(lines)


def emit_c(cries, pokemon_cries):
    lines = []
    lines.append('/* cry_data.c — Pokémon cry note data extracted from pokered-master.')
    lines.append(' * Generated by tools/extract_cries.py — DO NOT EDIT.')
    lines.append(' *')
    lines.append(' * ALWAYS refer to pokered-master audio/sfx/cry*.asm before modifying.')
    lines.append(' */')
    lines.append('#include "cry_data.h"')
    lines.append('')

    # Emit note arrays for each cry
    for idx, cry in enumerate(cries):
        cry_id = f'cry{idx:02x}'

        # Ch5 notes
        ch5 = cry['ch5']
        if ch5['notes']:
            lines.append(f'static const cry_sq_note_t k{cry_id}_ch5[] = {{')
            for (ln, vol, fade, freq) in ch5['notes']:
                lines.append(f'    {{ {ln:2d}, {vol:2d}, {fade:2d}, {freq:4d} }},')
            lines.append('};')

        # Ch6 notes
        ch6 = cry['ch6']
        if ch6['notes']:
            lines.append(f'static const cry_sq_note_t k{cry_id}_ch6[] = {{')
            for (ln, vol, fade, freq) in ch6['notes']:
                lines.append(f'    {{ {ln:2d}, {vol:2d}, {fade:2d}, {freq:4d} }},')
            lines.append('};')

        # Ch8 notes
        ch8 = cry['ch8']
        if ch8['notes']:
            lines.append(f'static const cry_noise_note_t k{cry_id}_ch8[] = {{')
            for (ln, vol, fade, nr43) in ch8['notes']:
                lines.append(f'    {{ {ln:2d}, {vol:2d}, {fade:2d}, 0x{nr43:02X} }},')
            lines.append('};')

        lines.append('')

    # Emit g_cry_defs[]
    lines.append('const cry_def_t g_cry_defs[NUM_BASE_CRIES] = {')
    for idx, cry in enumerate(cries):
        cry_id = f'cry{idx:02x}'
        ch5 = cry['ch5']
        ch6 = cry['ch6']
        ch8 = cry['ch8']

        ch5_notes = f'k{cry_id}_ch5' if ch5['notes'] else 'NULL'
        ch6_notes = f'k{cry_id}_ch6' if ch6['notes'] else 'NULL'
        ch8_notes = f'k{cry_id}_ch8' if ch8['notes'] else 'NULL'

        ch5_n = len(ch5['notes'])
        ch6_n = len(ch6['notes'])
        ch8_n = len(ch8['notes'])

        lines.append(f'    /* [{idx:2d}] cry{idx:02x} */')
        lines.append(f'    {{')
        lines.append(f'        /* ch5 */ {{ 0x{ch5["duty_pattern"]:02X}, {ch5["rotate_duty"]}, {ch5_n}, {ch5_notes} }},')
        lines.append(f'        /* ch6 */ {{ 0x{ch6["duty_pattern"]:02X}, {ch6["rotate_duty"]}, {ch6_n}, {ch6_notes} }},')
        lines.append(f'        /* ch8 */ {{ {ch8_n}, {ch8_notes} }},')
        lines.append(f'    }},')

    lines.append('};')
    lines.append('')

    # Emit g_pokemon_cries[]
    lines.append('const pokemon_cry_t g_pokemon_cries[NUM_POKEMON_CRIES] = {')
    lines.append('    /* { base_cry, pitch_mod, tempo_mod } */')
    for (base, pitch, tempo) in pokemon_cries:
        # pitch is stored as signed int8 in C
        pitch_signed = pitch if pitch < 128 else pitch - 256
        lines.append(f'    {{ {base:#04x}, {pitch_signed:4d}, 0x{tempo:02X} }},')
    lines.append('};')
    lines.append('')
    return '\n'.join(lines)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    cries = []
    for i in range(NUM_BASE_CRIES):
        fname = f'cry{i:02x}_1.asm'
        path = os.path.join(SFX_DIR, fname)
        if not os.path.exists(path):
            print(f'ERROR: missing {path}', file=sys.stderr)
            sys.exit(1)
        cry = parse_cry_file(path)
        cries.append(cry)
        print(f'  [{i:2d}] {fname}: ch5={len(cry["ch5"]["notes"])}n ch6={len(cry["ch6"]["notes"])}n ch8={len(cry["ch8"]["notes"])}n')

    pokemon_cries = parse_pokemon_cries(CRIES_ASM)
    print(f'\nParsed {len(pokemon_cries)} pokemon cry entries')

    h_content = emit_h(cries, pokemon_cries)
    c_content = emit_c(cries, pokemon_cries)

    with open(OUT_H, 'w', encoding='utf-8', newline='\n') as f:
        f.write(h_content)
    with open(OUT_C, 'w', encoding='utf-8', newline='\n') as f:
        f.write(c_content)

    print(f'\nWrote {OUT_H}')
    print(f'Wrote {OUT_C}')


if __name__ == '__main__':
    main()
