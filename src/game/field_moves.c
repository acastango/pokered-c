/* field_moves.c — Overworld HM field effects.
 *
 * Cut mirrors engine/overworld/cut.asm (UsedCut).
 *
 * Trigger flow (A-press variant, not original START→POKEMON path):
 *   Player presses A facing a cuttable tree block
 *   → YesNo "This looks like it can be cut.\nUse CUT?"
 *   → YES → "[mon] hacked\naway with CUT!" text
 *   → text closes → replace map block → rebuild view
 *
 * Badge requirement: CASCADE badge (Cerulean — matches original bit check).
 * Mon requirement: any party mon with MOVE_CUT in its move slots.
 *
 * Text strings match pokered-master data/text/text_7.asm:
 *   _UsedCutText: "[wNameBuffer] hacked\naway with CUT!"
 */
#include "field_moves.h"
#include "overworld.h"
#include "player.h"
#include "text.h"
#include "yesno.h"
#include "badge.h"
#include "constants.h"
#include "pokemon.h"
#include "npc.h"
#include "../platform/hardware.h"
#include "../platform/audio.h"
#include "../platform/display.h"
#include "../data/base_stats.h"
#include <stdio.h>
#include <string.h>

/* ---- Cut tree block swap table (data/tilesets/cut_tree_blocks.asm) ------ */
static const struct { uint8_t from; uint8_t to; } kCutBlocks[] = {
    {0x32, 0x6D}, {0x33, 0x6C}, {0x34, 0x6F}, {0x35, 0x4C},
    {0x60, 0x6E}, {0x0B, 0x0A}, {0x3C, 0x35}, {0x3F, 0x35},
    {0x3D, 0x36},
};
#define NUM_CUT_BLOCKS ((int)(sizeof(kCutBlocks)/sizeof(kCutBlocks[0])))

/* ---- State machine ------------------------------------------------------- */
typedef enum {
    FM_IDLE = 0,
    FM_CUT_CONFIRM,  /* YesNo open */
    FM_CUT_TEXT,     /* "[mon] hacked away with CUT!" text open */
    FM_CUT_ANIM,     /* Cut animation + SFX playing, then tile swap */
    FM_FLASH_TEXT,   /* "FLASH lit up the area!" text open */
    FM_FLASH_ANIM,   /* GBPalWhiteOutWithDelay3: brief white flash before normal palette */
} fm_state_t;

static fm_state_t s_state        = FM_IDLE;
static int        s_cut_fx       = 0;
static int        s_cut_fy       = 0;
static uint8_t    s_cut_newblock = 0;
static char       s_msg[64];
static int        s_anim_frame   = 0;

/* ---- Helpers ------------------------------------------------------------- */

static int find_cut_mon(void) {
    for (int i = 0; i < (int)wPartyCount; i++)
        for (int j = 0; j < 4; j++)
            if (wPartyMons[i].base.moves[j] == MOVE_CUT)
                return i;
    return -1;
}

/* Decode GB-encoded nickname → ASCII, falling back to species name. */
static void get_mon_name(int slot, char *buf, int buf_size) {
    const uint8_t *nick = wPartyMonNicks[slot];
    if (nick[0] != 0x00 && nick[0] != 0x50) {
        int out = 0;
        for (int i = 0; i < 10 && out < buf_size - 1; i++) {
            uint8_t c = nick[i];
            if (c == 0x50) break;
            if      (c >= 0x80 && c <= 0x99) buf[out++] = 'A' + (c - 0x80);
            else if (c >= 0xA0 && c <= 0xB9) buf[out++] = 'a' + (c - 0xA0);
            else if (c == 0x7F)              buf[out++] = ' ';
            else if (c >= 0xF6)              buf[out++] = '0' + (c - 0xF6);
        }
        buf[out] = '\0';
        if (out > 0) return;
    }
    uint8_t dex = gSpeciesToDex[wPartyMons[slot].base.species];
    snprintf(buf, buf_size, "%s", Pokemon_GetName(dex));
}

static int find_flash_mon(int slot) {
    if (slot < 0 || slot >= (int)wPartyCount) return 0;
    for (int j = 0; j < 4; j++)
        if (wPartyMons[slot].base.moves[j] == MOVE_FLASH)
            return 1;
    return 0;
}

/* ---- Public API ---------------------------------------------------------- */

int FieldMove_HasFlash(int slot) {
    return find_flash_mon(slot);
}

void FieldMove_TryFlash(int slot) {
    if (s_state != FM_IDLE) return;

    /* Boulder Badge required (Pewter gym — bit 0, mirrors BIT_BOULDERBADGE) */
    if (!(wObtainedBadges & (1u << BADGE_BOULDER))) return;

    /* Selected mon must know Flash */
    if (!find_flash_mon(slot)) return;

    /* Mirrors .flash in start_sub_menus.asm:
     * xor a / ld [wMapPalOffset], a  → clear darkness
     * PrintText .flashLightsAreaText → show text
     * GBPalWhiteOutWithDelay3        → palette (we apply immediately)
     * goBackToMap                    → done */
    gMapPalOffset = 0;  /* darkness cleared — palette applied after text + anim */
    Text_ShowASCII("FLASH lit up\nthe area!");
    s_state = FM_FLASH_TEXT;
}

void FieldMove_TryCut(void) {
    if (s_state != FM_IDLE) return;

    int fx, fy;
    Player_GetFacingTile(&fx, &fy);

    /* Check block in front for cuttable tree */
    uint8_t bid = Map_GetBlockAt(fx, fy);
    uint8_t new_block = 0xFF;
    for (int i = 0; i < NUM_CUT_BLOCKS; i++) {
        if (kCutBlocks[i].from == bid) { new_block = kCutBlocks[i].to; break; }
    }
    if (new_block == 0xFF) return;  /* not a cuttable tree */

    /* Require CASCADE badge (Cerulean gym — bit 1) */
    if (!(wObtainedBadges & (1u << BADGE_CASCADE))) return;

    /* Require a party mon that knows Cut */
    if (find_cut_mon() < 0) return;

    s_cut_fx       = fx;
    s_cut_fy       = fy;
    s_cut_newblock = new_block;

    YesNo_Show("This looks like\nit can be cut.\nUse CUT?");
    s_state = FM_CUT_CONFIRM;
}

int FieldMove_IsActive(void) {
    return s_state != FM_IDLE;
}

void FieldMove_Tick(void) {
    switch (s_state) {
        case FM_IDLE:
            break;

        case FM_CUT_CONFIRM:
            YesNo_Tick();
            if (!YesNo_IsOpen()) {
                if (YesNo_GetResult()) {
                    int slot = find_cut_mon();
                    char mon_name[16] = "MON";
                    if (slot >= 0) get_mon_name(slot, mon_name, sizeof(mon_name));
                    snprintf(s_msg, sizeof(s_msg), "%s hacked\naway with CUT!", mon_name);
                    Text_ShowASCII(s_msg);
                    s_state = FM_CUT_TEXT;
                } else {
                    s_state = FM_IDLE;
                }
            }
            break;

        case FM_CUT_TEXT:
            /* game.c handles text updates and early-returns while text is open,
             * so this case only runs on the frame after text closes. */
            Audio_PlaySFX_Cut();
            s_anim_frame = 0;
            Display_SetOverlayEnabled(1);
            s_state = FM_CUT_ANIM;
            break;

        case FM_FLASH_TEXT:
            if (!Text_IsOpen()) {
                /* Mirrors GBPalWhiteOutWithDelay3 = GBPalWhiteOut + Delay3:
                 * GBPalWhiteOut → rBGP=0/rOBP0=0/rOBP1=0 (instant all-white).
                 * Delay3 → wait exactly 3 frames for BG map to update.
                 * Then goBackToMap → LoadGBPal with wMapPalOffset=0 = normal. */
                Display_SetPalette(0x00, 0x00, 0x00);  /* instant white-out */
                s_anim_frame = 0;
                s_state = FM_FLASH_ANIM;
            }
            break;

        case FM_FLASH_ANIM:
            s_anim_frame++;
            if (s_anim_frame >= 3) {
                Display_LoadMapPalette();  /* snap to normal (gMapPalOffset=0) */
                s_state = FM_IDLE;
            }
            break;

        case FM_CUT_ANIM: {
            /* Flash the 2×2 tree tiles white for the first 12 frames,
             * then hold dark until the noise SFX finishes (~24 frames total).
             * Screen tile coords: stx = tree_world_tile_x - gCamX, same for Y. */
            int stx = s_cut_fx * 2 - gCamX;
            int sty = s_cut_fy * 2 - gCamY;
            if (s_anim_frame < 12) {
                /* Alternate full-white flash every 2 frames */
                uint32_t col = ((s_anim_frame & 2) == 0) ? 0xFFFFFF90u : 0x00000000u;
                if (stx >= 0 && stx + 1 < 20 && sty >= 0 && sty + 1 < 18) {
                    Display_SetOverlayTile(stx,     sty,     col);
                    Display_SetOverlayTile(stx + 1, sty,     col);
                    Display_SetOverlayTile(stx,     sty + 1, col);
                    Display_SetOverlayTile(stx + 1, sty + 1, col);
                }
            } else {
                Display_ClearOverlay();
            }
            s_anim_frame++;
            /* Wait until SFX finishes (24 frames) before swapping the tile */
            if (s_anim_frame >= 24 && !Audio_IsSFXPlaying()) {
                Display_ClearOverlay();
                Display_SetOverlayEnabled(0);
                Map_SetBlockAt(s_cut_fx, s_cut_fy, s_cut_newblock);
                s_state = FM_IDLE;
            }
            break;
        }
    }
}

void FieldMove_PostRender(void) {
    if (s_state == FM_CUT_CONFIRM)
        YesNo_PostRender();
}
