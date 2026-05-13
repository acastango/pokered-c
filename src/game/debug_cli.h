#pragma once
/* debug_cli.h — File-based game controller for Claude Code / scripted testing.
 *
 * Protocol:
 *   Write a command to  bugs/cli_cmd.txt
 *   Game processes it  (next frame poll cycle, every 30 frames)
 *   Game writes result to bugs/cli_state.txt and deletes cli_cmd.txt
 *
 * Commands (one per file):
 *   up [N]      — walk N tiles (default 1)
 *   down [N]
 *   left [N]
 *   right [N]
 *   a           — press A (interact/confirm)
 *   b           — press B (cancel/back)
 *   start       — press Start (menu)
 *   select      — press Select
 *   wait [N]    — wait N frames doing nothing (default 60)
 *   state       — just write state, no action
 *   animlab start [level] — start auto-play move animation test loop
 *   animlab stop          — stop auto-play (keeps current battle active)
 *   animlab status        — show animlab status in cli state/output
 *   hittrace on|off|reset|status — MoveHitTest diagnostic tracing
 *   replay record <name>  — capture full start state + per-frame inputs
 *   replay stop           — stop recording or playback
 *   replay play <name>    — load replay start state and auto-play frames
 *   replay status         — show replay runtime status
 *   movetestteam1         — set 6-mon party with 24 predefined move-anim test moves
 *
 * State file format: plain ASCII, grid + legend + party summary.
 */

void DebugCLI_Tick(void);   /* call once per GameTick */
void DebugCLI_PostRender(void); /* call after Map_BuildScrollView when needed */

/* ---- In-game ~ console -------------------------------------------- */
void DebugCLI_ConsoleOpen(void);
void DebugCLI_ConsoleClose(void);
int  DebugCLI_ConsoleIsOpen(void);
void DebugCLI_ConsoleAddChar(char c);
void DebugCLI_ConsoleBackspace(void);
void DebugCLI_ConsoleExecute(void);
void DebugCLI_ConsoleRender(void);  /* call each frame before display render */
void DebugCLI_ConsoleSetOverlayEnabled(int enabled);
void DebugCLI_ConsoleSetAlwaysOpen(int enabled);
const char *DebugCLI_ConsoleGetBuffer(void);

/* ---- Debug history feed (for debug-render sidebar) ---------------- */
int DebugCLI_GetHistoryCount(void);
const char *DebugCLI_GetHistoryLine(int newest_index); /* 0 = newest */
int DebugCLI_GetHistoryColor(int newest_index);        /* see CLI_HIST_COLOR_* */
int DebugCLI_IsReplayPlaying(void);
void DebugCLI_HistoryPushExternal(const char *line);
int DebugCLI_TriggerNpcWalkoff(void);
int DebugCLI_IsNpcWalkoffActive(void);
int DebugCLI_OnNpcInteracted(int npc_idx);

#define CLI_HIST_COLOR_DEFAULT 0
#define CLI_HIST_COLOR_OK      1
#define CLI_HIST_COLOR_ERROR   2
#define CLI_HIST_COLOR_LOG     3
