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
 *
 * State file format: plain ASCII, grid + legend + party summary.
 */

void DebugCLI_Tick(void);   /* call once per GameTick */

/* ---- In-game ~ console -------------------------------------------- */
void DebugCLI_ConsoleOpen(void);
void DebugCLI_ConsoleClose(void);
int  DebugCLI_ConsoleIsOpen(void);
void DebugCLI_ConsoleAddChar(char c);
void DebugCLI_ConsoleBackspace(void);
void DebugCLI_ConsoleExecute(void);
void DebugCLI_ConsoleRender(void);  /* call each frame before display render */
