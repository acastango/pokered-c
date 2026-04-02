#!/usr/bin/env bash
# launch_game.sh — Launch pokered.exe if not already running.
# Waits up to 15s for the CLI to become responsive.
#
# Run from anywhere inside the pokered repo.
# Exit codes: 0 = game ready, 1 = failed to start

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
GAME_EXE="$BUILD_DIR/pokered.exe"
TIMEOUT=15

# Already running?
if tasklist 2>/dev/null | grep -qi "pokered.exe"; then
    echo "[launch] pokered.exe already running"
    exit 0
fi

# Launch from build/ so the game finds bugs/cli_state.txt relative to its CWD
echo "[launch] starting pokered.exe..."
(cd "$BUILD_DIR" && "$GAME_EXE" --skip) &

# Poll game_cli.py for a response (it handles the bugs/ path correctly)
for i in $(seq 1 $TIMEOUT); do
    sleep 1
    if python "$REPO_ROOT/tools/game_cli.py" state 2>/dev/null | grep -q "==="; then
        echo "[launch] game ready (${i}s)"
        exit 0
    fi
done

echo "[launch] ERROR: game did not respond within ${TIMEOUT}s"
exit 1
