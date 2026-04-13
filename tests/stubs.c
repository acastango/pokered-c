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
void Display_SetOverlayEnabled(int on) { (void)on; }
void Display_ClearOverlay(void) {}
void Display_SetOverlayTile(int tx, int ty, uint32_t rgba) { (void)tx;(void)ty;(void)rgba; }
void Display_SetShakeOffset(int ox, int oy) { (void)ox;(void)oy; }
int  Display_SaveScreenshot(const char *path) { (void)path; return 0; }
void Display_SetBGP(uint8_t bgp) { (void)bgp; }
void Display_LoadMapPalette(void) {}
void Display_SetBandXPx(int row_start, int num_rows, int px) { (void)row_start;(void)num_rows;(void)px; }
void Display_SetOBP1(uint8_t obp1) { (void)obp1; }

void Display_SetWindowTile(int col, int row, uint8_t tile) { (void)col;(void)row;(void)tile; }

/* audio.h stubs */
int  Audio_Init(void)  { return 0; }
void Audio_Quit(void)  {}
void Audio_Update(void) {}
void Audio_WriteReg(int ch, int reg, uint8_t val) { (void)ch;(void)reg;(void)val; }
void Audio_SetWaveInstrument(int idx) { (void)idx; }
void Audio_PlaySFX_PressAB(void) {}
void Audio_PlaySFX_Ledge(void) {}
void Audio_PlaySFX_GoInside(void) {}
void Audio_PlaySFX_GoOutside(void) {}
void Audio_PlaySFX_StartMenu(void) {}
void Audio_PlaySFX_TurnOnPC(void) {}
void Audio_PlaySFX_EnterPC(void) {}
void Audio_PlaySFX_TurnOffPC(void) {}
void Audio_PlaySFX_BattleHit(uint8_t dmg_mult) { (void)dmg_mult; }
void Audio_PlaySFX_BallPoof(void) {}
void Audio_PlaySFX_Faint(void) {}
void Audio_PlaySFX_Run(void) {}
void Audio_PlaySFX_Cut(void) {}
void Audio_PlaySFX_Switch(void) {}
void Audio_PlaySFX_Denied(void) {}
void Audio_PlaySFX_HealingMachine(void) {}
void Audio_PlaySFX_LevelUp(void) {}
void Audio_PlaySFX_Purchase(void) {}
void Audio_PlaySFX_Collision(void) {}
void Audio_PlaySFX_GetKeyItem(void) {}
int  Audio_IsSFXPlaying_GetKeyItem(void) { return 0; }
void Audio_PlaySFX_SSAnneHorn(void) {}
int  Audio_IsSFXPlaying_SSAnneHorn(void) { return 0; }
int  Audio_IsSFXPlaying(void)            { return 0; }
void Audio_PlayCry(uint8_t species) { (void)species; }
int  Audio_IsCryPlaying(void) { return 0; }

/* save.h stubs */
int  Save_Load(void) { return -1; }  /* no save = new game */
int  Save_Save(void) { return  0; }

/* input.h stubs */
void Input_Init(void)  {}
void Input_Quit(void)  {}
void Input_Update(void) {}

/* audio.h stubs */
void Audio_PlaySFX_GetItem1(void) {}

/* battle_ui.h stubs */
void BattleUI_SetBadgeRecvText(const char *text) { (void)text; }
void BattleUI_SetBadgeInfoText(const char *text) { (void)text; }

/* block-ID overlay stubs */
void Display_SetBlockIDOverlay(int e) { (void)e; }
int  Display_GetBlockIDOverlay(void)  { return 0; }
void Display_SetBlockIDQueryFn(int (*fn)(int bx, int by)) { (void)fn; }
void Display_SetBlockIDCam(int tx, int ty) { (void)tx; (void)ty; }



