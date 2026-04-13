# Oak's Lab Post-Battle Timing: Staged Edits Only

These edits are intentionally **not applied** to the live source files.
They are staged here so they can be implemented later without colliding with
other in-flight work.

## Why the current timing feels off

After reviewing `pokered-master/scripts/OaksLab.asm`, the timing mismatch is not
just the 20-frame pause. The main drift points are:

1. The port arms the post-battle exit before the battle starts.
2. `game.c` restores map music immediately when the battle ends, but the ASM
   goes straight into the rival-exit aftermath instead.
3. `NPC_Load()` resets Blue to his map-default lab position on battle return,
   so the current aftermath starts from the wrong place unless we restore the
   saved pre-battle position.
4. The port splits `DelayFrames -> DisplayTextID -> Music_RivalAlternateStart ->
   MoveSprite` across too many states.

## Staged change set

### 1. Add a battle-return hook for Oak's Lab

Goal: let Oak's Lab learn that the rival battle has actually ended before
`game.c` restores map music and NPC defaults.

#### `src/game/oakslab_scripts.h`

Add:

```c
void OaksLabScripts_OnBattleReturn(uint8_t battle_result);
int OaksLabScripts_SuppressMapMusicRestore(void);
```

#### `src/game/game.c`

In the `SCENE_BATTLE` return path, after `NPC_Load(); Trainer_LoadMap(); Gate_LoadMap();`
and before the unconditional `Music_Play(Music_GetMapID(wCurMap));`, stage this flow:

```c
int suppress_map_music = 0;
OaksLabScripts_OnBattleReturn(saved_battle_result);
if (OaksLabScripts_SuppressMapMusicRestore())
    suppress_map_music = 1;

if (!suppress_map_music)
    Music_Play(Music_GetMapID(wCurMap));
```

Intent:

- normal battles keep existing behavior
- Oak's Lab rival battle suppresses the immediate map-music restart so the
  post-battle script can own the transition timing

### 2. Capture and restore Blue's pre-battle position

Goal: mimic the ASM's `GetSpritePosition1` / `SetSpritePosition1` pairing.

#### `src/game/npc.h`

Add:

```c
void NPC_SetTilePos(int i, int tx, int ty);
```

#### `src/game/npc.c`

Add a small helper near `NPC_GetTilePos`:

```c
void NPC_SetTilePos(int i, int tx, int ty) {
    if (i < 0 || i >= npc_count) return;
    npc_x[i] = (uint8_t)tx;
    npc_y[i] = (uint8_t)ty;
    npc_px_off[i] = 0;
    npc_py_off[i] = 0;
    npc_walk_frames[i] = 0;
}
```

This keeps the change local and avoids widening the engine API beyond what the
Oak's Lab aftermath needs.

#### `src/game/oakslab_scripts.c`

Replace the current `gRivalExitPending`-only approach with explicit battle-state
tracking:

```c
static int gRivalBattleActive = 0;
static int gRivalPostBattlePending = 0;
static int gSavedRivalBattleX = 0;
static int gSavedRivalBattleY = 0;
```

At `OLS_BATTLE_TRIGGER`, before queuing the battle:

```c
NPC_GetTilePos(OAKSLAB_RIVAL_IDX, &gSavedRivalBattleX, &gSavedRivalBattleY);
gRivalBattleActive = 1;
gRivalPostBattlePending = 0;
```

Do **not** set `EVENT_BATTLED_RIVAL_IN_OAKS_LAB` here.

### 3. Make post-battle start only after an actual victory

#### `src/game/oakslab_scripts.c`

Add:

```c
void OaksLabScripts_OnBattleReturn(uint8_t battle_result) {
    if (wCurMap != MAP_OAKS_LAB) return;
    if (!gRivalBattleActive) return;

    gRivalBattleActive = 0;

    if (battle_result == BATTLE_OUTCOME_TRAINER_VICTORY)
        gRivalPostBattlePending = 1;
}

int OaksLabScripts_SuppressMapMusicRestore(void) {
    return (wCurMap == MAP_OAKS_LAB && gRivalPostBattlePending);
}
```

Update the map-load / idle checks to use `gRivalPostBattlePending` instead of
the old pre-battle `gRivalExitPending` sentinel.

### 4. Tighten the post-battle state machine to match the ASM

Replace the current staged flow:

- `OLS_POST_BATTLE_SETUP`
- `OLS_POST_BATTLE_DELAY`
- `OLS_POST_BATTLE_TEXT`
- `OLS_RIVAL_EXIT_WAIT`

with a tighter sequence:

- `OLS_POST_BATTLE_SETUP`
- `OLS_POST_BATTLE_STARTS_EXIT`
- `OLS_PLAYER_WATCH_RIVAL_EXIT`
- `OLS_RIVAL_GONE`

#### `OLS_POST_BATTLE_SETUP`

This state should do the `OaksLabRivalEndBattleScript` work in one shot:

```c
gScriptedMovement = 1;
set_player_facing(DIR_UP);
NPC_SetTilePos(OAKSLAB_RIVAL_IDX, gSavedRivalBattleX, gSavedRivalBattleY);
NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_DOWN);
Pokemon_HealParty();
SetEvent(EVENT_BATTLED_RIVAL_IN_OAKS_LAB);
gDelay = 20;
gState = OLS_POST_BATTLE_STARTS_EXIT;
return;
```

Notes:

- restoring Blue's saved tile before facing him down is the local equivalent of
  `SetSpritePosition1`
- this is where the event should be set, matching the ASM

#### `OLS_POST_BATTLE_STARTS_EXIT`

Use a single state for the full `OaksLabRivalStartsExitScript` beat:

```c
if (gDelay-- > 0) return;

if (!gTextStarted) {
    Text_ShowASCII(kRivalSmellYouLater);
    gTextStarted = 1;
    return;
}

if (Text_IsOpen()) return;

Music_Play(MUSIC_MEET_RIVAL); /* best current stand-in for Music_RivalAlternateStart */
NPC_SetFacing(OAKSLAB_RIVAL_IDX, DIR_DOWN);
gRivalExitStep = 0;
gState = OLS_PLAYER_WATCH_RIVAL_EXIT;
return;
```

Important staging note:

- the current engine does not expose a `Music_RivalAlternateStart` equivalent,
  so this staged edit still uses `Music_Play(MUSIC_MEET_RIVAL)` unless a better
  audio entry point is added later
- the key timing fix is that music restart and exit start happen immediately
  after the text closes, without an extra idle phase

#### `OLS_PLAYER_WATCH_RIVAL_EXIT`

Mirror `OaksLabPlayerWatchRivalExitScript` more literally:

```c
if (!NPC_IsWalking(OAKSLAB_RIVAL_IDX)) {
    if (gRivalExitStep >= RIVAL_EXIT_STEPS) {
        gState = OLS_RIVAL_GONE;
        return;
    }

    NPC_DoScriptedStep(OAKSLAB_RIVAL_IDX, DIR_DOWN);
    gRivalExitStep++;
}

/* Mirror wNPCNumScriptedSteps thresholds from the ASM. */
if (gRivalExitStep == 0) {
    if ((int)wXCoord == 4) set_player_facing(DIR_RIGHT);
    else                   set_player_facing(DIR_LEFT);
} else if (gRivalExitStep == 1) {
    set_player_facing(DIR_DOWN);
}

return;
```

This keeps the timing closer to the ASM's `cp $5` / `cp $4` checks by anchoring
the facing transitions to the first two exit-step windows instead of a more
abstract helper state.

#### `OLS_RIVAL_GONE`

Keep:

```c
NPC_HideSprite(OAKSLAB_RIVAL_IDX);
gRivalPostBattlePending = 0;
gScriptedMovement = 0;
Music_Play(Music_GetMapID(wCurMap));
gState = OLS_IDLE;
```

## Files intended to change when this is eventually applied

- `src/game/oakslab_scripts.c`
- `src/game/oakslab_scripts.h`
- `src/game/game.c`
- `src/game/npc.c`
- `src/game/npc.h`

## What stays untouched

- imported `RIVAL1` trainer data
- Oak's Lab battle selection logic
- Oak's Lab rival approach-before-battle logic
- later Pokédex scene
