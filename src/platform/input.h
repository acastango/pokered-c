#pragma once
#include <stdint.h>

/* input.h — SDL joypad → GB hJoyInput mapping */

void Input_Init(void);
void Input_Update(void);    /* call once per frame; updates hJoy* globals */
void Input_Quit(void);

/* Input recorder / playback.
 * Recording captures one byte of hJoyInput per frame to a binary file.
 * Playback feeds those bytes back instead of reading the keyboard.
 * F9 in main.c toggles recording; F10 toggles playback. */
void Input_StartRecording(const char *path);
void Input_StopRecording(void);
void Input_StartPlayback(const char *path);
void Input_StopPlayback(void);
int  Input_IsRecording(void);
int  Input_IsPlaying(void);
