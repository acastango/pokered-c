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

/* Text-print chirp — mirrors SFX_PRESS_AB from audio/sfx/press_ab_1.asm */
void Audio_PlaySFX_PressAB(void);
