/* stubs.c — No-op stubs for SDL2-dependent platform functions.
 * Allows game logic to be tested without a display or audio device. */
#include <stdint.h>

/* display.h stubs */
int  Display_Init(void)  { return 0; }
void Display_Quit(void)  {}
void Display_Render(void) {}
void Display_LoadTileset(const uint8_t *g, int n) { (void)g; (void)n; }
void Display_LoadTile(uint8_t id, const uint8_t *g) { (void)id; (void)g; }
void Display_LoadSpriteTile(uint8_t id, const uint8_t *g) { (void)id; (void)g; }
void Display_SetPalette(uint8_t a, uint8_t b, uint8_t c) { (void)a;(void)b;(void)c; }
void Display_GetTile(uint8_t id, uint8_t out[16]) { (void)id; (void)out; }
void Display_RenderScrolled(int px, int py, const uint8_t *m, int s) { (void)px;(void)py;(void)m;(void)s; }

/* audio.h stubs */
int  Audio_Init(void)  { return 0; }
void Audio_Quit(void)  {}
void Audio_Update(void) {}
void Audio_WriteReg(int ch, int reg, uint8_t val) { (void)ch;(void)reg;(void)val; }
void Audio_PlaySFX_PressAB(void) {}

/* save.h stubs */
int  Save_Load(void) { return -1; }  /* no save = new game */
int  Save_Save(void) { return  0; }

/* input.h stubs */
void Input_Init(void)  {}
void Input_Quit(void)  {}
void Input_Update(void) {}
