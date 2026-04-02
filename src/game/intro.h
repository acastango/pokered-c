#pragma once

/* intro.h — New-game intro sequence (Professor Oak's speech).
 *
 * Mirrors engine/movie/oak_speech/oak_speech.asm:OakSpeech.
 *
 * Flow:
 *   Intro_Start() → text boxes → player/rival name init → bedroom warp
 *
 * When Intro_IsActive() goes from 1 to 0, the bedroom map is loaded and
 * player data is fully initialized.  game.c then calls the enter-map path
 * (Player_Init / NPC_Load / etc.) using wCurMap / wXCoord / wYCoord.
 */

void Intro_Start(void);
int  Intro_IsActive(void);
void Intro_Tick(void);
