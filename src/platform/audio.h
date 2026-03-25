#pragma once
#include <stdint.h>

/* audio.h — GB APU emulation via SDL2 audio callback
 *
 * Synthesis layer (audio.c):
 *   Audio_Init / Audio_Quit  — SDL2 device lifecycle
 *   Audio_WriteReg           — write a GB APU register (triggers synthesis)
 *   Audio_PlaySFX_PressAB    — fire the text-print chirp SFX
 *   Audio_Update             — per-frame tick (calls Music_Update + SFX state)
 *
 * Sequencer layer (music.c):
 *   Music_Play / Music_Stop / Music_Update — see music.h
 *
 * GB freq formula:
 *   hz = 131072 / (2048 - gb_freq11)   for CH0/CH1 (square wave)
 *   hz =  65536 / (2048 - gb_freq11)   for CH2 (wave channel)
 */

#define AUDIO_SAMPLE_RATE  44100
#define AUDIO_CHANNELS     1       /* mono */
#define AUDIO_BUFFER_SIZE  512

int  Audio_Init(void);
void Audio_Quit(void);

/* Called once per frame (60 Hz) by game.c */
void Audio_Update(void);

/* Write a GB APU hardware register.
 * channel: 0-3  (0=CH1 square, 1=CH2 square, 2=CH3 wave, 3=CH4 noise)
 * reg:     1-4  (NRx1=duty/len, NRx2=vol/env, NRx3=freq_lo, NRx4=freq_hi+trigger) */
void Audio_WriteReg(int channel, int reg, uint8_t value);

/* Load a wave instrument (0-4) into CH3 wave RAM.
 * Patterns from audio/wave_samples.asm; idx out of range defaults to 0. */
void Audio_SetWaveInstrument(int idx);

/* Text-print chirp — mirrors SFX_PRESS_AB from audio/sfx/press_ab_1.asm */
void Audio_PlaySFX_PressAB(void);

/* Start menu open — mirrors SFX_START_MENU (home/start_menu.asm) */
void Audio_PlaySFX_StartMenu(void);

/* Ledge-hop bloop — mirrors SFX_LEDGE (SFX_Ledge_1_Ch5) from audio/sfx/ledge_1.asm */
void Audio_PlaySFX_Ledge(void);

/* Building/cave enter and exit — mirrors SFX_GO_INSIDE / SFX_GO_OUTSIDE
 * (home/overworld.asm PlayMapChangeSound, noise channel Ch8) */
void Audio_PlaySFX_GoInside(void);
void Audio_PlaySFX_GoOutside(void);

/* Battle hit sounds — mirrors PlayApplyingAttackSound (engine/battle/animations.asm).
 * dmg_mult: wDamageMultipliers & 0x7F
 *   0     → immune, no SFX
 *   1-9   → SFX_NOT_VERY_EFFECTIVE
 *   10    → SFX_DAMAGE (normal hit)
 *   11+   → SFX_SUPER_EFFECTIVE */
void Audio_PlaySFX_BattleHit(uint8_t dmg_mult);

/* Pokeball open poof — mirrors SFX_BALL_POOF (Ch5 sweep + Ch8 noise). */
void Audio_PlaySFX_BallPoof(void);

/* Faint sequence — mirrors SFX_FAINT_FALL then SFX_FAINT_THUD.
 * Falls on Ch5 (square+sweep) then thuds on Ch5+Ch8 (square+noise). */
void Audio_PlaySFX_Faint(void);

/* Run away — mirrors SFX_RUN (audio/sfx/run.asm, noise channel Ch8). */
void Audio_PlaySFX_Run(void);

/* Level-up jingle — mirrors SFX_LEVEL_UP (audio/sfx/level_up.asm).
 * Plays on Ch5 (ch[0]) and Ch6 (ch[1]) simultaneously. ~132 frames. */
void Audio_PlaySFX_LevelUp(void);

/* Healing machine SFX — mirrors SFX_HEALING_MACHINE (audio/sfx/healing_machine_1.asm).
 * Short 3-note pitch-sweep on Ch5 (ch[0]).  Call once per party member
 * with ~30-frame gaps between calls (mirrors AnimateHealingMachine loop). */
void Audio_PlaySFX_HealingMachine(void);

/* Purchase kaching — mirrors SFX_PURCHASE (audio/sfx/purchase_1.asm).
 * Two-channel ascending arpeggio on Ch5 (ch[0]) and Ch6 (ch[1]).
 * Total duration ~12 frames. */
void Audio_PlaySFX_Purchase(void);

/* Pokémon cry — mirrors PlayCry (home/pokemon.asm:145).
 * species: 1-based internal species ID (SPECIES_* constants).
 * Looks up CryData for pitch/tempo modifiers, plays 3-channel cry.
 * Suspends music ch[0] and ch[1]; resumes when done. */
void Audio_PlayCry(uint8_t species);
