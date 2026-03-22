#pragma once
#include <stdint.h>

/* Music IDs — match MUSIC_* constants in pokered music_constants.asm */
#define MUSIC_NONE            0
#define MUSIC_PALLET_TOWN     1
#define MUSIC_POKECENTER      2
#define MUSIC_GYM             3
#define MUSIC_CITIES1         4
#define MUSIC_CITIES2         5
#define MUSIC_CELADON         6
#define MUSIC_CINNABAR        7
#define MUSIC_VERMILION       8
#define MUSIC_LAVENDER        9
#define MUSIC_SS_ANNE         10
#define MUSIC_ROUTES1         11
#define MUSIC_ROUTES2         12
#define MUSIC_ROUTES3         13
#define MUSIC_ROUTES4         14
#define MUSIC_INDIGO_PLATEAU  15
#define MUSIC_OAKS_LAB        16
#define MUSIC_DUNGEON1        17
#define MUSIC_DUNGEON2        18
#define MUSIC_DUNGEON3        19
#define MUSIC_POKEMON_TOWER   20
#define MUSIC_SILPH_CO        21
#define MUSIC_SAFARI_ZONE     22
#define MUSIC_TITLE           23
#define MUSIC_JIGGLYPUFF      24

/* Play a music track (stops any current music first).
 * music_id: one of MUSIC_* above; MUSIC_NONE stops playback. */
void Music_Play(uint8_t music_id);
void Music_Stop(void);

/* Advance the sequencer one frame (call at 60 Hz from Audio_Update). */
void Music_Update(void);

/* Suspend/resume a music channel so SFX can temporarily use it.
 * ResumeChannel re-fires the current note so music continues seamlessly. */
void Music_SuspendChannel(int ch);
void Music_ResumeChannel(int ch);

/* Look up which music ID belongs to a given map ID. */
uint8_t Music_GetMapID(uint8_t map_id);
