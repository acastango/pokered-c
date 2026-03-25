#!/usr/bin/env python3
"""Convert pokered music ASM files to C note_evt_t arrays.

Usage: python extract_audio.py > ../src/data/music_data_gen.h
"""

import re, os, sys

# Note pitch constants: 0=C_, 1=C#, ... 11=B_
NOTE_IDX = {
    'C_': 0, 'C#': 1, 'Db': 1,
    'D_': 2, 'D#': 3, 'Eb': 3,
    'E_': 4,
    'F_': 5, 'F#': 6, 'Gb': 6,
    'G_': 7, 'G#': 8, 'Ab': 8,
    'A_': 9, 'A#': 10, 'Bb': 10,
    'B_': 11,
}

# GB base 16-bit signed pitch values (from audio/notes.asm)
GB_PITCH_BASE = [
    0xF82C, 0xF89D, 0xF907, 0xF96B, 0xF9CA, 0xFA23,
    0xFA77, 0xFAC7, 0xFB12, 0xFB58, 0xFB9B, 0xFBDA,
]

def calc_freq(note_idx, pokered_octave, perfect_pitch=False):
    de = GB_PITCH_BASE[note_idx]
    if de >= 0x8000:
        de -= 0x10000
    for _ in range(pokered_octave - 1):
        de >>= 1
    d = ((de >> 8) & 0xFF)
    e = de & 0xFF
    d = (d + 8) & 0xFF
    freq = ((d & 7) << 8) | e
    if perfect_pitch:
        freq = (freq + 1) & 0x7FF
    return freq

def calc_delay(note_len, speed, tempo, frac):
    full  = note_len * speed * tempo + frac
    delay = (full >> 8) & 0xFF
    frac  = full & 0xFF
    return max(1, delay), frac


class State:
    def __init__(self):
        self.tempo        = 256
        self.speed        = 6
        self.volume       = 7
        self.duty         = 2
        self.octave       = 4
        self.frac         = 0
        self.perfect_pitch= False
        self.env_nibble   = 0  # low nibble of NRx2: (dir<<3)|pace, 0 = no envelope
        self.wave_inst    = 0  # Ch3 wave instrument index (0-4), stored in duty field

    def copy(self):
        s = State()
        s.__dict__.update(self.__dict__)
        return s


def preprocess(raw_lines):
    """Strip comments and blank lines, return (flat_lines, label_map).
    label_map: {label_name: line_index_in_flat}.

    Local labels (starting with '.') are scoped to their enclosing global label,
    e.g. '.sub1' under 'Music_Pokecenter_Ch2::' is keyed as
    'Music_Pokecenter_Ch2.sub1'.  This prevents channels sharing the same local
    label name (e.g. .sub1, .loop1) from resolving to the wrong definition.
    """
    flat          = []
    labels        = {}
    current_scope = ''   # most recent non-local label, trailing ':' stripped
    for raw in raw_lines:
        s = raw.strip()
        if ';' in s:
            s = s[:s.index(';')].rstrip()
        s = s.strip()
        if not s:
            continue
        if s.endswith(':'):
            lname = s[:-1]
            if lname.startswith('.'):
                # local label — key it under the current global scope
                key = current_scope + lname
            else:
                # global label — update scope (strip any trailing ':' from '::')
                current_scope = lname.rstrip(':')
                key = lname
            labels[key] = len(flat)
            flat.append(s)          # keep label in flat for reference
        else:
            flat.append(s)
    return flat, labels


def parse_block(flat, start_idx, state, labels, stop_on_ret=False, scope='', is_wave=False):
    """Parse note/command lines starting at start_idx.
    Returns (events, next_idx, loop_start_in_events).
    If stop_on_ret=True, stop at sound_ret (used for subroutines).

    scope — the enclosing channel label (e.g. 'Music_Pokecenter_Ch2').
    Local labels starting with '.' are resolved as scope+label, matching
    the scoping applied in preprocess().
    """
    events          = []
    loop_start      = -1
    label_event_idx = {}   # local label name → event index when label was seen
    i               = start_idx

    def resolve(lbl):
        """Return the labels-dict key for lbl, applying scope for local labels."""
        return (scope + lbl) if lbl.startswith('.') else lbl

    while i < len(flat):
        s = flat[i]; i += 1

        # label lines — track for inner-loop expansion
        if s.endswith(':'):
            lname = s[:-1]
            label_event_idx[lname] = len(events)
            if '.mainloop' in s:
                loop_start = len(events)
            continue

        # commands
        if s.startswith('tempo '):
            m = re.match(r'tempo\s+(\d+)', s)
            if m: state.tempo = int(m.group(1))

        elif s.startswith('volume '):
            pass  # master volume ignored

        elif s.startswith('duty_cycle '):
            m = re.match(r'duty_cycle\s+(\d+)', s)
            if m: state.duty = int(m.group(1))

        elif s.startswith('note_type '):
            m = re.match(r'note_type\s+(\d+)\s*,\s*(\d+)\s*,\s*([-\d]+)', s)
            if m:
                state.speed  = int(m.group(1))
                state.volume = int(m.group(2))
                fade_raw = int(m.group(3))
                if is_wave:
                    # Ch3: fade low nibble = wave instrument index; no envelope
                    state.wave_inst  = fade_raw & 0xF
                    state.env_nibble = 0
                else:
                    # Matches the dn macro: negative fade → bit3=1 (increase) + magnitude
                    if fade_raw < 0:
                        state.env_nibble = 0x8 | ((-fade_raw) & 0x7)  # increase
                    else:
                        state.env_nibble = fade_raw & 0xF              # decrease or 0

        elif s.startswith('octave '):
            m = re.match(r'octave\s+(\d+)', s)
            if m: state.octave = int(m.group(1))

        elif s == 'toggle_perfect_pitch':
            state.perfect_pitch = not state.perfect_pitch

        elif s.startswith('vibrato') or s.startswith('pitch_slide') \
             or s.startswith('stereo_panning') or s.startswith('duty_cycle_pattern') \
             or s.startswith('pitch_sweep'):
            pass  # ignored

        elif s.startswith('note '):
            m = re.match(r'note\s+([A-G][b#_]),\s*(\d+)', s)
            if m:
                name     = m.group(1)
                note_len = int(m.group(2))
                if name in NOTE_IDX:
                    freq = calc_freq(NOTE_IDX[name], state.octave,
                                     state.perfect_pitch)
                    delay, state.frac = calc_delay(
                        note_len, state.speed, state.tempo, state.frac)
                    env_byte = (state.volume << 4) | state.env_nibble
                    duty_val = state.wave_inst if is_wave else state.duty
                    events.append((freq, delay, duty_val, state.volume, env_byte))

        elif s.startswith('rest '):
            m = re.match(r'rest\s+(\d+)', s)
            if m:
                note_len = int(m.group(1))
                delay, state.frac = calc_delay(
                    note_len, state.speed, state.tempo, state.frac)
                duty_val = state.wave_inst if is_wave else state.duty
                events.append((0, delay, duty_val, 0, 0))

        elif s.startswith('sound_call '):
            m = re.match(r'sound_call\s+(\S+)', s)
            if m:
                sub_label = m.group(1)
                key = resolve(sub_label)
                if key in labels:
                    sub_start = labels[key] + 1  # skip the label line itself
                    sub_events, _, _ = parse_block(flat, sub_start, state,
                                                   labels, stop_on_ret=True,
                                                   scope=scope, is_wave=is_wave)
                    events.extend(sub_events)

        elif s.startswith('sound_ret'):
            if stop_on_ret:
                break

        elif s.startswith('sound_loop'):
            m = re.match(r'sound_loop\s+(\d+)\s*,\s*(\S+)', s)
            if m:
                count = int(m.group(1))
                lbl   = m.group(2)
                if count > 0:
                    # Inner repeat: play the loop body `count` more times (N+1 total).
                    # label_event_idx uses the raw local name (no scope needed —
                    # it tracks labels seen in *this* parse call only).
                    seg_start = label_event_idx.get(lbl)
                    if seg_start is not None:
                        seg = list(events[seg_start:])
                        for _ in range(count):
                            events.extend(seg)
                    # Continue — execution falls through after the inner loop
                else:
                    # count == 0: outer infinite loop, handled by loop_start
                    break
            else:
                break  # malformed

    return events, i, loop_start


def extract_global_tempo(raw_lines, ch1_label=None):
    """Find the first 'tempo N' encountered when playing from the main Ch1 entry.
    In pokered, 'tempo' writes a single global register shared by all channels.
    It is always set in Ch1's preamble and applies to every channel in the song.

    Some songs (e.g. cities1) have an AlternateTempo variant before the main
    Ch1 label that sets a different tempo.  Scanning the whole file blindly
    picks up that wrong value.  When ch1_label is provided we skip everything
    before that label so we only see the tempo the song actually uses."""
    if ch1_label:
        in_main = False
        for line in raw_lines:
            s = line.strip()
            if ';' in s:
                s = s[:s.index(';')].strip()
            if s == ch1_label + '::':
                in_main = True
                continue
            if in_main:
                m = re.match(r'tempo\s+(\d+)', s)
                if m:
                    return int(m.group(1))
                # Hit the next channel label — tempo must be a file-level default
                if s.endswith('::') and s != ch1_label + '::':
                    break
    # Fallback: scan whole file for first tempo command
    for line in raw_lines:
        s = line.strip()
        if ';' in s:
            s = s[:s.index(';')].strip()
        m = re.match(r'tempo\s+(\d+)', s)
        if m:
            return int(m.group(1))
    return 256  # default when no tempo command present


def parse_channel(raw_lines, channel_label, global_tempo=None, is_wave=False):
    """Parse one music channel. Returns (events, loop_start)."""
    flat, labels = preprocess(raw_lines)

    # Find the channel entry point
    ch_start = None
    for idx, s in enumerate(flat):
        if s == channel_label + '::':
            ch_start = idx + 1
            break
    if ch_start is None:
        return [], -1

    state = State()
    if global_tempo is not None:
        state.tempo = global_tempo   # shared across all channels in this song
    events, _, loop_start = parse_block(flat, ch_start, state, labels,
                                        stop_on_ret=True, scope=channel_label,
                                        is_wave=is_wave)
    return events, loop_start


# -----------------------------------------------------------
# C code emitters
# -----------------------------------------------------------
def emit_array(c_name, events, loop_start):
    lines = [f'static const note_evt_t {c_name}[] = {{']
    for idx, (freq, frames, duty, vol, env_byte) in enumerate(events):
        lm = '  /* loop */' if idx == loop_start else ''
        lines.append(f'    {{{freq:5}, {frames:3}, {duty}, {vol}, 0x{env_byte:02X}}},' + lm)
    lines.append(f'}};  /* {len(events)} events, loop@{loop_start} */')
    return '\n'.join(lines)

def emit_ch_data(c_name, arr_name, count, loop_start):
    return (f'static const ch_data_t {c_name} = '
            f'{{ {arr_name}, {count}, {loop_start} }};')


# -----------------------------------------------------------
# Map music ID table
# -----------------------------------------------------------
MUSIC_ID = {
    'MUSIC_NONE':           0,
    'MUSIC_PALLET_TOWN':    1,
    'MUSIC_POKECENTER':     2,
    'MUSIC_GYM':            3,
    'MUSIC_CITIES1':        4,
    'MUSIC_CITIES2':        5,
    'MUSIC_CELADON':        6,
    'MUSIC_CINNABAR':       7,
    'MUSIC_VERMILION':      8,
    'MUSIC_LAVENDER':       9,
    'MUSIC_SS_ANNE':        10,
    'MUSIC_ROUTES1':        11,
    'MUSIC_ROUTES2':        12,
    'MUSIC_ROUTES3':        13,
    'MUSIC_ROUTES4':        14,
    'MUSIC_INDIGO_PLATEAU': 15,
    'MUSIC_OAKS_LAB':       16,
    'MUSIC_DUNGEON1':       17,
    'MUSIC_DUNGEON2':       18,
    'MUSIC_DUNGEON3':       19,
    'MUSIC_POKEMON_TOWER':  20,
    'MUSIC_SILPH_CO':       21,
    'MUSIC_SAFARI_ZONE':    22,
    'MUSIC_TITLE':          23,
    'MUSIC_JIGGLYPUFF':     24,
    'MUSIC_WILD_BATTLE':    25,
    'MUSIC_PKMN_HEALED':   26,
}

def parse_songs(songs_asm_path):
    ids = []
    with open(songs_asm_path) as f:
        for line in f:
            s = line.strip()
            if ';' in s:
                s = s[:s.index(';')].strip()
            if s.startswith('db '):
                parts = s[3:].split(',')
                if parts:
                    sym = parts[0].strip()
                    ids.append(MUSIC_ID.get(sym, 0))
    return ids


# -----------------------------------------------------------
# Main
# -----------------------------------------------------------
def main():
    base      = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             '..', 'pokered-master')
    music_dir = os.path.join(base, 'audio', 'music')

    songs = [
        ('pallettown', 'Music_PalletTown',  3),
        ('routes1',    'Music_Routes1',     3),
        ('oakslab',    'Music_OaksLab',     3),
        ('wildbattle', 'Music_WildBattle',  3),
        ('cities1',    'Music_Cities1',     3),
        ('routes3',    'Music_Routes3',     3),
        ('pokecenter', 'Music_Pokecenter',  3),
        ('dungeon2',   'Music_Dungeon2',    3),
        ('cities2',    'Music_Cities2',     3),
        ('dungeon3',          'Music_Dungeon3',          3),
        ('defeatedwildmon',   'Music_DefeatedWildMon',   3),
        ('defeatedtrainer',   'Music_DefeatedTrainer',   3),
        ('defeatedgymleader', 'Music_DefeatedGymLeader', 3),
        ('pkmnhealed',        'Music_PkmnHealed',        3),
        ('trainerbattle',    'Music_TrainerBattle',    3),
    ]

    print('/* AUTO-GENERATED by tools/extract_audio.py — do not edit manually */')
    print('#pragma once')
    print('#include <stdint.h>')
    print()
    print('typedef struct { uint16_t freq; uint16_t frames;')
    print('                 uint8_t duty;  uint8_t volume; uint8_t env_byte; } note_evt_t;')
    print('typedef struct { const note_evt_t *notes; int count;')
    print('                 int loop_start; } ch_data_t;')
    print()

    for stem, label_base, num_ch in songs:
        with open(os.path.join(music_dir, stem + '.asm')) as f:
            raw = f.readlines()
        global_tempo = extract_global_tempo(raw, f'{label_base}_Ch1')
        short = label_base.replace('Music_', '')
        for ch in range(1, num_ch + 1):
            ch_label  = f'{label_base}_Ch{ch}'
            arr_name  = f'k{short}_Ch{ch}_notes'
            data_name = f'k{short}_Ch{ch}'
            events, ls = parse_channel(raw, ch_label, global_tempo=global_tempo,
                                       is_wave=(ch == 3))
            if not events:
                print(f'/* WARNING: {ch_label} produced 0 events */')
                # emit a single silent event to avoid empty array
                events = [(0, 1, 0, 0, 0)]
                ls = 0
            print(emit_array(arr_name, events, ls))
            print(emit_ch_data(data_name, arr_name, len(events), ls))
            print()

    # Map music ID table
    songs_path = os.path.join(base, 'data', 'maps', 'songs.asm')
    ids = parse_songs(songs_path)
    print(f'/* Map music IDs: index = map ID, 0 = none/unknown */')
    print(f'static const uint8_t kMapMusicID[{len(ids)}] = {{')
    for i in range(0, len(ids), 8):
        chunk = ids[i:i+8]
        print('    ' + ', '.join(str(x) for x in chunk) + ',')
    print('};')

if __name__ == '__main__':
    main()
