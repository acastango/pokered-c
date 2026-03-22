#pragma once
#include <stdint.h>

/* input.h — SDL joypad → GB hJoyInput mapping */

void Input_Init(void);
void Input_Update(void);    /* call once per frame; updates hJoy* globals */
void Input_Quit(void);
