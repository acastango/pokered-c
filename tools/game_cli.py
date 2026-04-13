#!/usr/bin/env python3
"""game_cli.py — File-based controller for the pokered debug CLI.

Usage:
  Interactive:   python tools/game_cli.py
  One-shot:      python tools/game_cli.py "left 3"
  Script file:   python tools/game_cli.py --script path/to/cmds.txt

Commands:
  up/down/left/right [N]   walk N tiles (default 1)
  a / interact             press A
  b / back                 press B
  start / menu             press Start
  select                   press Select
  wait [N]                 idle N frames (default 60)
  state                    refresh display without acting
  teleport <map> [x y]     jump to map (uses existing teleport.txt)
  giveitem <name|id> [qty] add item to bag (e.g. giveitem pokeflute, giveitem potion 5)
  givetm <n>               give TM n (1-50)
  givehm <n>               give HM n (1-5)
  givemon <dex> [level]    add pokemon to party (dex = national dex number)
  givebadge <n>            set badge bit n (0=boulder … 7=earth)
  setflag <n>              set event flag n (decimal or 0x hex)
  clearflag <n>            clear event flag n
  sethealth <slot> <hp>    set current HP for party slot (1-based)
  setlevel <slot> <lvl>    set level for party slot (1-based)
  healparty                full HP + clear status on all party mons
  setmoney <amount>        set money (0-999999)
  listbag                  print current bag contents
  quit / exit              leave interactive mode

The game must be running. Commands are written to bugs/cli_cmd.txt;
state is read back from bugs/cli_state.txt.
"""

import sys
import os
import time
import argparse

TIMEOUT    = 10.0  # seconds to wait for state update

def _find_bugs_dir():
    """Return the bugs/ dir relative to wherever pokered.exe lives."""
    import os
    candidates = [
        "bugs",                   # already in build/
        "build/bugs",             # from repo root
        "../build/bugs",          # from tools/
    ]
    # Prefer the dir whose sibling contains pokered.exe
    for c in candidates:
        parent = os.path.normpath(os.path.join(c, ".."))
        if os.path.exists(os.path.join(parent, "pokered.exe")):
            return c
    return candidates[1]          # fallback: build/bugs

_BUGS = _find_bugs_dir()
CMD_FILE   = f"{_BUGS}/cli_cmd.txt"
STATE_FILE = f"{_BUGS}/cli_state.txt"
TELE_FILE  = f"{_BUGS}/teleport.txt"


def ensure_bugs_dir():
    os.makedirs(_BUGS, exist_ok=True)


def send_command(cmd: str) -> bool:
    """Write cmd file and wait for game to consume it (it deletes the file)."""
    ensure_bugs_dir()
    # Get current state mtime so we can detect an update
    try:
        before = os.path.getmtime(STATE_FILE)
    except FileNotFoundError:
        before = 0.0

    with open(CMD_FILE, "w") as f:
        f.write(cmd.strip() + "\n")

    deadline = time.time() + TIMEOUT
    while time.time() < deadline:
        # Command file deleted = game picked it up
        if not os.path.exists(CMD_FILE):
            # Now wait for state file to be newer than before
            inner = time.time() + TIMEOUT
            while time.time() < inner:
                try:
                    after = os.path.getmtime(STATE_FILE)
                    if after > before:
                        return True
                except FileNotFoundError:
                    pass
                time.sleep(0.05)
            return True  # cmd was consumed even if state write timed out
        time.sleep(0.05)

    # Timed out — remove stale cmd file
    try:
        os.remove(CMD_FILE)
    except FileNotFoundError:
        pass
    return False


NAMED_LOCATIONS = {
    # Towns / Cities
    "pallet":           (0,  10, 18),
    "pallet_town":      (0,  10, 18),
    "viridian":         (1,  24, 40),
    "viridian_city":    (1,  24, 40),
    "pewter":           (2,  16, 18),
    "pewter_city":      (2,  16, 18),
    "cerulean":         (3,  28, 18),
    "cerulean_city":    (3,  28, 18),
    "vermilion":        (5,  28, 24),
    "vermilion_city":   (5,  28, 24),
    "lavender":         (4,  14, 18),
    "lavender_town":    (4,  14, 18),
    "celadon":          (6,  44, 24),
    "celadon_city":     (6,  44, 24),
    "fuchsia":          (7,  28, 24),
    "fuchsia_city":     (7,  28, 24),
    "cinnabar":         (8,  14, 18),
    "cinnabar_island":  (8,  14, 18),
    "saffron":          (10, 28, 28),
    "saffron_city":     (10, 28, 28),
    # Gyms
    "viridian_gym":     (52,  8,  9),
    "pewter_gym":       (54,  8,  9),
    "cerulean_gym":     (65,  8,  9),
    "vermilion_gym":    (92,  8,  9),
    "celadon_gym":      (135, 8,  9),
    "fuchsia_gym":      (166, 8,  9),
    "saffron_gym":      (178, 8,  9),
    "cinnabar_gym":     (234, 8,  9),
    # Key locations
    "oaks_lab":         (37,  12, 11),
    "viridian_forest":  (51,  14, 40),
    "mt_moon":          (59,  14, 10),
    "rock_tunnel":      (155, 14, 10),
    "pokemon_tower":    (142, 8,  9),
    "silph_co":         (200, 8,  9),
    "safari_zone":      (217, 28, 20),
    # Routes
    "route_1":  (12, 14, 70), "route_2":  (13, 14, 10),
    "route_3":  (14, 14, 10), "route_4":  (15, 14, 10),
    "route_5":  (16, 14, 10), "route_6":  (17, 14, 70),
    "route_7":  (18, 14, 10), "route_8":  (19, 14, 70),
    "route_9":  (20, 14, 10), "route_10": (21, 14, 10),
    "route_11": (22, 14, 10), "route_12": (23, 14, 10),
    "route_24": (33, 14, 10), "route_25": (34, 14, 10),
}


def send_teleport(args_str: str):
    """Send teleport via the cli_cmd.txt mechanism (same as all other commands)."""
    ensure_bugs_dir()
    args = args_str.strip()

    # Named location lookup
    key = args.split()[0].lower().replace("-", "_") if args else ""
    if key and not key[0].isdigit():
        if key not in NAMED_LOCATIONS:
            print(f"[cli] Unknown location: {key}")
            print(f"[cli] Known: {', '.join(sorted(NAMED_LOCATIONS))}")
            return
        m, x, y = NAMED_LOCATIONS[key]
        args = f"{m} {x} {y}"

    # Convert any hex values (e.g. 0x52) to decimal for the game's sscanf
    parts = args.split()
    parts = [str(int(p, 0)) if p.startswith("0x") or p.startswith("0X") else p for p in parts]
    cmd = "teleport " + " ".join(parts)

    print(f"[cli] Teleport: {cmd}")
    send_command(cmd)
    time.sleep(0.3)
    show_state()


def show_state():
    try:
        with open(STATE_FILE) as f:
            print(f.read())
    except FileNotFoundError:
        print("[cli] No state file yet — send a command first (or type 'state').")


def run_command(raw: str) -> bool:
    """Execute one command string. Returns False to quit."""
    cmd = raw.strip()
    if not cmd or cmd.startswith("#"):
        return True
    if cmd in ("quit", "exit", "q"):
        return False

    verb = cmd.split()[0].lower()

    if verb == "teleport":
        rest = cmd[len("teleport"):].strip()
        send_teleport(rest)
        return True

    ok = send_command(cmd)
    if not ok:
        print("[cli] Timed out — is the game running?")
    show_state()
    return True


def interactive():
    print("pokered game CLI  (type 'help' for commands, 'quit' to exit)")
    print("Game must be running. Commands go to bugs/cli_cmd.txt.\n")
    show_state()
    while True:
        try:
            raw = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if raw == "help":
            print(__doc__)
            continue
        if not run_command(raw):
            break


def run_script(path: str):
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            print(f">>> {line}")
            if not run_command(line):
                break


def main():
    parser = argparse.ArgumentParser(description="pokered game CLI")
    parser.add_argument("command", nargs="?", help="single command to run")
    parser.add_argument("--script", help="run commands from a file")
    args = parser.parse_args()

    if args.script:
        run_script(args.script)
    elif args.command:
        run_command(args.command)
        show_state()
    else:
        interactive()


if __name__ == "__main__":
    main()
