---
name: npc-post-battle-reposition
description: Fix for scripted NPC appearing at wrong position after returning from battle
user-invocable: false
---

# NPC Post-Battle Position Fix

## The Problem

When a scripted NPC walks to a position and a battle starts, `game.c` reloads the map after
the battle ends. `NPC_Load()` resets all NPCs to their default positions from `event_data.c`.
If the NPC walked 5 tiles right before the battle, after the battle it snaps back to its spawn
position, not where it stood during the encounter.

This causes two visible bugs:
1. The NPC sprite appears at the wrong spot (spawn point instead of battle position)
2. If there is a second NPC entry at the same default coordinates, a duplicate sprite appears

## The Fix (established pattern — see cerulean_scripts.c and route22_scripts.c)

### 1. Save the final tile position when the walk completes

```c
static int g_rival_stop_x = 0;
static int g_rival_stop_y = 0;

// In Tick(), when walk steps hit 0 (just before transitioning away from WALK state):
NPC_GetTilePos(NPC_IDX, &g_rival_stop_x, &g_rival_stop_y);
```

### 2. Restore position in OnMapLoad for post-battle states

```c
void FooScripts_OnMapLoad(void) {
    // Post-battle states: NPC_Load() already ran and reset positions.
    // Re-show and reposition the NPC to where it stood during the battle.
    if (g_state == STATE_POST_BATTLE ||
        g_state == STATE_EXIT_WALK   ||
        g_state == STATE_CLEANUP) {
        NPC_ShowSprite(NPC_IDX);
        NPC_SetTilePos(NPC_IDX, g_rival_stop_x, g_rival_stop_y);
        NPC_SetFacing(NPC_IDX, FACING_AFTER_BATTLE);  // NPC_Load resets facing too
        return;
    }
    // ... normal hide logic ...
}
```

### 3. Hide any unused sibling sprites

If `event_data.c` has multiple NPC entries at the same position (e.g., RIVAL1 and RIVAL2
both at the default coords), always explicitly hide the unused ones in every branch of
`OnMapLoad`, including the post-battle branch:

```c
NPC_HideSprite(UNUSED_NPC_IDX);  // e.g., index 1 for Route 22 RIVAL2
NPC_ShowSprite(ACTIVE_NPC_IDX);
NPC_SetTilePos(ACTIVE_NPC_IDX, g_rival_stop_x, g_rival_stop_y);
```

## Why This Happens

`game.c` calls `Map_Load()` + `NPC_Load()` after every battle. `NPC_Load()` iterates
`gMapEvents[wCurMap].npcs` and places each NPC at its `event_data.c` coordinates.
The scripted walk only updated the NPC's runtime state — it was never persisted. The
`OnMapLoad` callback runs after `NPC_Load()`, making it the right place to override
the reset positions for any NPC that moved during a scripted sequence.
